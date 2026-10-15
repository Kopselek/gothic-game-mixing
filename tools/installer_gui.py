from __future__ import annotations

import contextlib
import io
import os
import queue
import sys
import tempfile
import threading
import traceback
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

# install.py, the mashups' descriptions and the tools they import run inside this process, so
# PyInstaller bundles them with everything they import.
import install
import union


APP_NAME = "Gothic Mashup Installer"


def release_dir() -> str:
    """The release package places the EXE and the mashups' plugins (CJMovement.vdf, Witcher.vdf) side by side."""
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class QueueWriter(io.TextIOBase):
    def __init__(self, output_queue: queue.Queue[str]):
        self.output_queue = output_queue

    def write(self, text: str) -> int:
        if text:
            self.output_queue.put(text)
        return len(text)

    def flush(self) -> None:
        pass


class InstallerApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title(APP_NAME)
        self.root.geometry("760x560")
        self.root.minsize(660, 500)

        self.output_queue: queue.Queue[str] = queue.Queue()
        self.running = False

        self.game_var = tk.StringVar(value=next(iter(install.MASHUPS)))
        # each game keeps its own folder, so switching back and forth loses nothing
        self.game_dirs = {name: tk.StringVar() for name in install.MASHUPS}
        self.gothic_var = tk.StringVar()
        self.lang_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Choose the game, select both game folders, then click Install.")

        main = ttk.Frame(root, padding=16)
        main.pack(fill="both", expand=True)
        main.columnconfigure(1, weight=1)
        main.rowconfigure(7, weight=1)

        ttk.Label(
            main,
            text="Gothic 1 mashups",
            font=("Segoe UI", 15, "bold"),
        ).grid(row=0, column=0, columnspan=3, sticky="w", pady=(0, 6))
        ttk.Label(
            main,
            text="Gothic 1 is the base. Choose the game to combine it with, then select your existing "
                 "folders of both games. The installer does not include either game.",
            wraplength=700,
        ).grid(row=1, column=0, columnspan=3, sticky="w", pady=(0, 16))

        ttk.Label(main, text="Combine Gothic 1 with").grid(
            row=2, column=0, sticky="w", padx=(0, 10), pady=5
        )
        games = ttk.Frame(main)
        games.grid(row=2, column=1, columnspan=2, sticky="w", pady=5)
        self.game_buttons = []
        for mashup in install.MASHUPS.values():
            button = ttk.Radiobutton(
                games,
                text=f"{mashup.TITLE} (play as {mashup.HERO})",
                value=mashup.NAME,
                variable=self.game_var,
                command=self.show_game,
            )
            button.pack(side="left", padx=(0, 18))
            self.game_buttons.append(button)

        self.game_label = ttk.Label(main)
        self.game_label.grid(row=3, column=0, sticky="w", padx=(0, 10), pady=5)
        self.game_entry = ttk.Entry(main)
        self.game_entry.grid(row=3, column=1, sticky="ew", pady=5)
        ttk.Button(main, text="Browse…", command=self.choose_game_dir).grid(
            row=3, column=2, padx=(8, 0), pady=5
        )

        ttk.Label(main, text="Gothic 1 folder").grid(
            row=4, column=0, sticky="w", padx=(0, 10), pady=5
        )
        ttk.Entry(main, textvariable=self.gothic_var).grid(
            row=4, column=1, sticky="ew", pady=5
        )
        ttk.Button(main, text="Browse…", command=self.choose_gothic).grid(
            row=4, column=2, padx=(8, 0), pady=5
        )

        # only for a mashup whose hero speaks a dub you choose (The Witcher 3)
        self.lang_label = ttk.Label(main)
        self.lang_label.grid(row=5, column=0, sticky="w", padx=(0, 10), pady=5)
        self.lang_box = ttk.Combobox(main, textvariable=self.lang_var, width=8, state="readonly")
        self.lang_box.grid(row=5, column=1, sticky="w", pady=5)

        ttk.Label(main, text="Installer log").grid(
            row=6, column=0, columnspan=3, sticky="w", pady=(14, 5)
        )
        self.log = tk.Text(main, height=12, wrap="word", state="disabled")
        self.log.grid(row=7, column=0, columnspan=3, sticky="nsew")
        scrollbar = ttk.Scrollbar(main, orient="vertical", command=self.log.yview)
        scrollbar.grid(row=7, column=3, sticky="ns")
        self.log.configure(yscrollcommand=scrollbar.set)

        bottom = ttk.Frame(main)
        bottom.grid(row=8, column=0, columnspan=3, sticky="ew", pady=(12, 0))
        bottom.columnconfigure(0, weight=1)
        ttk.Label(bottom, textvariable=self.status_var).grid(
            row=0, column=0, sticky="w"
        )
        self.install_button = ttk.Button(
            bottom, text="Install", command=self.start_install
        )
        self.install_button.grid(row=0, column=1, sticky="e", padx=(8, 0))
        ttk.Button(bottom, text="Close", command=self.root.destroy).grid(
            row=0, column=2, sticky="e", padx=(8, 0)
        )

        self.show_game()
        self.root.after(100, self.poll_output)

    def mashup(self):
        return install.MASHUPS[self.game_var.get()]

    def show_game(self) -> None:
        """The game folder (and dub) rows for the chosen game."""
        mashup = self.mashup()
        self.game_label.configure(text=f"{mashup.TITLE} folder")
        self.game_entry.configure(textvariable=self.game_dirs[mashup.NAME])
        if mashup.DEFAULT_LANG:
            self.lang_label.configure(text=f"{mashup.HERO}'s voice (dub)")
            self.update_dubs()
            self.lang_label.grid()
            self.lang_box.grid()
        else:
            self.lang_label.grid_remove()
            self.lang_box.grid_remove()

    def update_dubs(self) -> None:
        mashup = self.mashup()
        langs = mashup.dubs(self.game_dirs[mashup.NAME].get().strip()) or [mashup.DEFAULT_LANG]
        self.lang_box.configure(values=langs)
        if self.lang_var.get() not in langs:
            self.lang_var.set(mashup.DEFAULT_LANG if mashup.DEFAULT_LANG in langs
                              else ("en" if "en" in langs else langs[0]))

    def choose_game_dir(self) -> None:
        mashup = self.mashup()
        path = filedialog.askdirectory(title=f"Select {mashup.GAME_FOLDER}")
        if path:
            self.game_dirs[mashup.NAME].set(path)
            if mashup.DEFAULT_LANG:
                self.update_dubs()

    def choose_gothic(self) -> None:
        path = filedialog.askdirectory(title="Select the Gothic 1 folder")
        if path:
            self.gothic_var.set(path)

    def append_log(self, text: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.configure(state="disabled")

    def poll_output(self) -> None:
        try:
            while True:
                self.append_log(self.output_queue.get_nowait())
        except queue.Empty:
            pass
        self.root.after(100, self.poll_output)

    def start_install(self) -> None:
        if self.running:
            return

        mashup = self.mashup()
        game_dir = self.game_dirs[mashup.NAME].get().strip()
        gothic = self.gothic_var.get().strip()
        lang = (self.lang_var.get().strip() or mashup.DEFAULT_LANG) if mashup.DEFAULT_LANG else None
        plugin = install.find_plugin(mashup, release_dir())

        if not game_dir or not os.path.isdir(game_dir):
            messagebox.showerror(APP_NAME, f"Select your {mashup.TITLE} folder: {mashup.GAME_FOLDER}.")
            return
        if not gothic or not os.path.isdir(gothic):
            messagebox.showerror(APP_NAME, "Select a valid Gothic 1 folder.")
            return
        if not plugin:
            messagebox.showerror(
                APP_NAME,
                f"{mashup.PLUGIN}.vdf was not found next to the installer.\n\n"
                f"Keep {install.INSTALLER_EXE} and the .vdf files in the same folder "
                "or extract all files from the release ZIP.",
            )
            return

        # everything but Union first, so a wrong folder isn't met with an offer to install Union into it
        problems = install.check(gothic, mashup.NAME, game_dir, install_union=not union.installed(gothic), lang=lang)
        if problems:
            messagebox.showerror(APP_NAME, "Can't install:\n\n- " + "\n- ".join(problems))
            return

        others = [m for m in install.installed_mashups(gothic) if m is not mashup]
        if others and not messagebox.askyesno(
            APP_NAME,
            f"This Gothic folder has {others[0].HERO} ({others[0].TITLE}) installed. A Gothic folder "
            f"holds one mashup at a time, so installing {mashup.TITLE} removes it first: Gothic.dat and "
            f"Gothic.ini go back to how they were before it, and its files are deleted.\n\nContinue?",
        ):
            return

        argv = ["--game", mashup.NAME, "--game-dir", game_dir, "--gothic", gothic, "--plugin", plugin]
        if lang:
            argv += ["--lang", lang]
        if not union.installed(gothic):
            if not messagebox.askyesno(
                APP_NAME,
                "Union, which loads the mod's plugin, is not installed in this Gothic folder, or Steam "
                "put Gothic's own DLLs back over it (it does when it updates or verifies the game).\n\n"
                "Download Union 1.0m (from its author's GitHub archive) and install it now?\n"
                "The files it replaces are backed up in system\\_backup_before_union.",
            ):
                messagebox.showinfo(
                    APP_NAME,
                    f"Install Union 1.0m for Gothic 1 yourself ({union.UNION_THREAD}), "
                    "then click Install again.",
                )
                return
            argv.append("--install-union")

        self.running = True
        self.install_button.configure(state="disabled")
        for button in self.game_buttons:
            button.configure(state="disabled")
        self.status_var.set(f"Installing {mashup.TITLE}… (this takes several minutes)")
        self.append_log(f"Starting installation: Gothic 1 + {mashup.TITLE}…\n")

        thread = threading.Thread(target=self.run_installer, args=(argv,), daemon=True)
        thread.start()

    def run_installer(self, argv: list[str]) -> None:
        writer = QueueWriter(self.output_queue)
        success = False
        try:
            with contextlib.redirect_stdout(writer), contextlib.redirect_stderr(writer):
                install.main(argv)
            success = True
        except SystemExit as exc:
            if exc.code in (None, 0):
                success = True
            elif isinstance(exc.code, str):
                self.output_queue.put(f"\n{exc.code}\n")
            else:
                self.output_queue.put(f"\nInstaller exited with code {exc.code}.\n")
        except Exception:
            self.output_queue.put("\nInstallation failed:\n" + traceback.format_exc() + "\n")
        self.root.after(0, lambda: self.finish(success))

    def finish(self, success: bool) -> None:
        self.running = False
        self.install_button.configure(state="normal")
        for button in self.game_buttons:
            button.configure(state="normal")
        if success:
            self.status_var.set("Installation finished.")
            self.append_log("\nInstallation finished.\n")
            messagebox.showinfo(APP_NAME, "Installation finished.")
        else:
            self.status_var.set("Installation failed. Check the log above.")
            messagebox.showerror(
                APP_NAME,
                "Installation failed. Check the log above for details.",
            )


def selftest(log_path: str | None) -> int:
    """Check a built installer (CI runs it before publishing): every mashup's tools import with what
    they need (numpy, Pillow), and Union downloads, verifies and unpacks into a scratch folder."""
    with contextlib.ExitStack() as stack:
        out = stack.enter_context(open(log_path, "w", encoding="utf-8")) if log_path else sys.stdout
        stack.enter_context(contextlib.redirect_stdout(out))
        stack.enter_context(contextlib.redirect_stderr(out))
        try:
            for mashup in install.MASHUPS.values():
                mashup.selftest()
            import tex

            print("numpy for textures:", "bundled" if tex._numpy() else "missing (slower textures)")
            with tempfile.TemporaryDirectory() as gothic:
                union.install(gothic)
                if not union.installed(gothic):
                    raise RuntimeError("Union files missing after union.install")
            print("selftest: ok")
            return 0
        except BaseException:
            traceback.print_exc()
            print("selftest: FAILED")
            return 1


def main() -> None:
    if sys.argv[1:2] == ["--selftest"]:
        sys.exit(selftest(sys.argv[2] if len(sys.argv) > 2 else None))
    root = tk.Tk()
    try:
        ttk.Style().theme_use("vista")
    except tk.TclError:
        pass
    InstallerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
