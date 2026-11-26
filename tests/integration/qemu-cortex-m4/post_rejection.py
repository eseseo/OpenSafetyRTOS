"""Pass only for the expected pre-scheduler rejection and emulator exit code."""
import pathlib
import subprocess
import sys
import tempfile

qemu, objcopy, source, fault = sys.argv[1:]
with tempfile.TemporaryDirectory() as work:
    image = pathlib.Path(source)
    if fault == "image":
        data = pathlib.Path(work) / "data"
        damaged = pathlib.Path(work) / "damaged.elf"
        subprocess.run([objcopy, "--dump-section", f".data={data}", image, damaged], check=True)
        payload = bytearray(data.read_bytes())
        payload[-1] ^= 1
        data.write_bytes(payload)
        subprocess.run([objcopy, "--update-section", f".data={data}", damaged], check=True)
        image = damaged
    result = subprocess.run([qemu, "-machine", "mps2-an386", "-display", "none",
                             "-monitor", "none", "-serial", "none", "-semihosting-config",
                             "enable=on,target=native", "-kernel", image],
                            capture_output=True, text=True, timeout=10)
    output = result.stdout + result.stderr
    print(output, end="")
    if result.returncode != 2 or f"POST rejected {fault} before scheduler start" not in output:
        sys.exit(1)
