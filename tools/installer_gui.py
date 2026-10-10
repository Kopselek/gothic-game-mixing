from __future__ import annotations

import contextlib
import io
import os
import queue
import runpy
import sys
import threading
import traceback
import tkinter as tk
from tkinter import filedialog, messagebox, ttk


APP_NAME = "Gothic GTA Installer"


def resource_path(*parts: str) -> str:
    """Return a path to bundled resources (PyInstaller) or source-tree resources."""
    if getattr(sys, "frozen", False) and hasattr(sys, "_MEIPASS"):
        base = sys._MEIPASS
    else:
        base = os.path.dirname(os.path.abspath(__file__))
        # In a source checkout this file lives in tools/, so tools/ is the base.
    return os.path.join(base, *parts)


def release_dir() -> str:
    """The release package places the EXE and CJMovement.vdf side by side."""
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
        self.root.geometry("720x500")
        self.root.minsize(620, 440)

        self.output_queue: queue.Queue[str] = queue.Queue()
        self.running = False

        self.gta_var = tk.StringVar()
        self.gothic_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Select both game folders, then click Install.")

        main = ttk.Frame(root, padding=16)
        main.pack(fill="both", expand=True)
        main.columnconfigure(1, weight=1)
        main.rowconfigure(5, weight=1)

        ttk.Label(
            main,
            text="Gothic 1 + GTA: San Andreas",
            font=("Segoe UI", 15, "bold"),
        ).grid(row=0, column=0, columnspan=3, sticky="w", pady=(0, 6))
        ttk.Label(
            main,
            text="Choose your existing game folders. The installer does not include either game.",
            wraplength=660,
        ).grid(row=1, column=0, columnspan=3, sticky="w", pady=(0, 16))

        ttk.Label(main, text="GTA: San Andreas folder").grid(
            row=2, column=0, sticky="w", padx=(0, 10), pady=5
        )
        ttk.Entry(main, textvariable=self.gta_var).grid(
            row=2, column=1, sticky="ew", pady=5
        )
        ttk.Button(main, text="Browse…", command=self.choose_gta).grid(
            row=2, column=2, padx=(8, 0), pady=5
        )

        ttk.Label(main, text="Gothic 1 folder").grid(
            row=3, column=0, sticky="w", padx=(0, 10), pady=5
        )
        ttk.Entry(main, textvariable=self.gothic_var).grid(
            row=3, column=1, sticky="ew", pady=5
        )
        ttk.Button(main, text="Browse…", command=self.choose_gothic).grid(
            row=3, column=2, padx=(8, 0), pady=5
        )

        ttk.Label(main, text="Installer log").grid(
            row=4, column=0, columnspan=3, sticky="w", pady=(14, 5)
        )
        self.log = tk.Text(main, height=12, wrap="word", state="disabled")
        self.log.grid(row=5, column=0, columnspan=3, sticky="nsew")
        scrollbar = ttk.Scrollbar(main, orient="vertical", command=self.log.yview)
        scrollbar.grid(row=5, column=3, sticky="ns")
        self.log.configure(yscrollcommand=scrollbar.set)

        bottom = ttk.Frame(main)
        bottom.grid(row=6, column=0, columnspan=3, sticky="ew", pady=(12, 0))
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

        self.root.after(100, self.poll_output)

    def choose_gta(self) -> None:
        path = filedialog.askdirectory(title="Select the GTA: San Andreas folder")
        if path:
            self.gta_var.set(path)

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

        gta = self.gta_var.get().strip()
        gothic = self.gothic_var.get().strip()
        plugin = os.path.join(release_dir(), "CJMovement.vdf")
        installer_script = resource_path("tools", "install.py")

        if not gta or not os.path.isdir(gta):
            messagebox.showerror(APP_NAME, "Select a valid GTA: San Andreas folder.")
            return
        if not gothic or not os.path.isdir(gothic):
            messagebox.showerror(APP_NAME, "Select a valid Gothic 1 folder.")
            return
        if not os.path.isfile(plugin):
            messagebox.showerror(
                APP_NAME,
                "CJMovement.vdf was not found next to the installer.\n\n"
                "Keep GothicGTAInstaller.exe and CJMovement.vdf in the same folder "
                "or extract both files from the release ZIP.",
            )
            return
        if not os.path.isfile(installer_script):
            messagebox.showerror(
                APP_NAME,
                "The bundled tools/install.py script was not found. "
                "Rebuild the installer with tools included.",
            )
            return

        self.running = True
        self.install_button.configure(state="disabled")
        self.status_var.set("Installing…")
        self.append_log("Starting installation…\n")

        thread = threading.Thread(
            target=self.run_installer,
            args=(installer_script, gta, gothic, plugin),
            daemon=True,
        )
        thread.start()

    def run_installer(self, installer_script: str, gta: str, gothic: str, plugin: str) -> None:
        old_argv = sys.argv[:]
        old_path = sys.path[:]
        writer = QueueWriter(self.output_queue)
        try:
            tools_dir = os.path.dirname(installer_script)
            if tools_dir not in sys.path:
                sys.path.insert(0, tools_dir)
            sys.argv = [
                "install.py",
                "--gta", gta,
                "--gothic", gothic,
                "--plugin", plugin,
            ]
            with contextlib.redirect_stdout(writer), contextlib.redirect_stderr(writer):
                runpy.run_path(installer_script, run_name="__main__")
        except SystemExit as exc:
            if exc.code not in (None, 0):
                self.output_queue.put(f"\nInstaller exited with code {exc.code}.\n")
                self.root.after(0, lambda: self.finish(False))
            else:
                self.root.after(0, lambda: self.finish(True))
        except Exception:
            self.output_queue.put("\nInstallation failed:\n" + traceback.format_exc() + "\n")
            self.root.after(0, lambda: self.finish(False))
        else:
            self.root.after(0, lambda: self.finish(True))
        finally:
            sys.argv = old_argv
            sys.path[:] = old_path

    def finish(self, success: bool) -> None:
        self.running = False
        self.install_button.configure(state="normal")
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


def main() -> None:
    root = tk.Tk()
    try:
        ttk.Style().theme_use("vista")
    except tk.TclError:
        pass
    InstallerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
