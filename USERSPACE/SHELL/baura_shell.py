
import sys
from vfs.vfs_core.vfs import BauraVFS

class BauraShell:
    def __init__(self):
        self.vfs = BauraVFS()
        self.vfs.mount("C:", "BAURAFS") # Default mount
        self.current_dir = "PC/C:/"
        self.running = True

    def run(self):
        print("BAURA OS Shell v0.1")
        print("Type 'help' for commands.\n")

        while self.running:
            try:
                cmd_input = input(f"BAURA@{self.current_dir}> ").strip()
                if not cmd_input: continue

                parts = cmd_input.split(' ', 1)
                cmd = parts[0].lower()
                args = parts[1] if len(parts) > 1 else ""

                if cmd == "exit":
                    self.running = False
                elif cmd == "help":
                    print("Commands: list <path>, read <path>, write <path> <text>, cd <path>, exit")
                elif cmd == "list":
                    path = args if args else self.current_dir
                    print(self.vfs.list_dir(path))
                elif cmd == "read":
                    print(self.vfs.read(args))
                elif cmd == "write":
                    # Format: write <path> <content>
                    try:
                        p, c = args.split(' ', 1)
                        print(self.vfs.write(p, c))
                    except ValueError:
                        print("Usage: write <path> <content>")
                elif cmd == "cd":
                    # Simplified CD
                    if args.startswith("PC/"):
                        self.current_dir = args
                    else:
                        print("Error: Use native BAURA paths (PC/C:/...)")
                else:
                    print(f"Unknown command: {cmd}")
            except Exception as e:
                print(f"Shell Error: {e}")

if __name__ == "__main__":
    shell = BauraShell()
    shell.run()
