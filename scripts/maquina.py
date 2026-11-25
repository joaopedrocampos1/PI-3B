"""Especificação da máquina para results/maquina.txt (#58), de forma portátil.

Substitui lscpu, nproc, free e /etc/os-release do antigo bateria.sh: lê
/proc no Linux, sysctl no macOS e a API do Windows por ctypes. Quando uma
informação não pode ser obtida, grava "desconhecido" em vez de falhar.
"""

import ctypes
import datetime
import os
import platform
import re
import subprocess
import sys

DESCONHECIDO = "desconhecido"


def _saida(cmd):
    try:
        return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def cpu():
    if sys.platform.startswith("linux"):
        try:
            with open("/proc/cpuinfo", encoding="utf-8") as f:
                for linha in f:
                    if linha.startswith("model name"):
                        return linha.split(":", 1)[1].strip()
        except OSError:
            pass
    elif sys.platform == "darwin":
        return _saida(["sysctl", "-n", "machdep.cpu.brand_string"]) or DESCONHECIDO
    elif os.name == "nt":
        try:
            import winreg
            chave = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                   r"HARDWARE\DESCRIPTION\System\CentralProcessor\0")
            return winreg.QueryValueEx(chave, "ProcessorNameString")[0].strip()
        except OSError:
            pass
    return platform.processor() or DESCONHECIDO


def memoria_bytes():
    if sys.platform.startswith("linux"):
        try:
            with open("/proc/meminfo", encoding="utf-8") as f:
                for linha in f:
                    if linha.startswith("MemTotal:"):
                        return int(linha.split()[1]) * 1024
        except OSError:
            return None
    if sys.platform == "darwin":
        valor = _saida(["sysctl", "-n", "hw.memsize"])
        return int(valor) if valor.isdigit() else None
    if os.name == "nt":
        class Status(ctypes.Structure):
            _fields_ = [("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong),
                        ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                        ("ullTotalPageFile", ctypes.c_ulonglong),
                        ("ullAvailPageFile", ctypes.c_ulonglong),
                        ("ullTotalVirtual", ctypes.c_ulonglong),
                        ("ullAvailVirtual", ctypes.c_ulonglong),
                        ("ullAvailExtendedVirtual", ctypes.c_ulonglong)]
        s = Status()
        s.dwLength = ctypes.sizeof(Status)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(s)):
            return s.ullTotalPhys
    return None


def memoria():
    """No formato do `free -h`, que o maquina.txt versionado já usa: 15Gi."""
    b = memoria_bytes()
    if b is None:
        return DESCONHECIDO
    for unidade, fator in (("Ti", 1 << 40), ("Gi", 1 << 30), ("Mi", 1 << 20)):
        if b >= fator:
            v = b / fator
            return f"{v:.1f}{unidade}" if v < 10 else f"{int(v)}{unidade}"
    return f"{b}B"


def sistema():
    if sys.platform.startswith("linux"):
        try:
            nome = platform.freedesktop_os_release().get("PRETTY_NAME", "Linux")
        except (OSError, AttributeError):   # AttributeError: Python < 3.10
            nome = "Linux"
        return f"{nome}, kernel {platform.release()}"
    if sys.platform == "darwin":
        return f"macOS {platform.mac_ver()[0]}, kernel {platform.release()}"
    if os.name == "nt":
        return f"Windows {platform.release()} (versão {platform.version()})"
    return platform.platform()


def compilador():
    saida = _saida(["gcc", "--version"])
    return saida.splitlines()[0] if saida else DESCONHECIDO


def flags(makefile="Makefile"):
    try:
        with open(makefile, encoding="utf-8") as f:
            for linha in f:
                m = re.match(r"CFLAGS\s*=\s*(.*)", linha)
                if m:
                    return m.group(1).strip()
    except OSError:
        pass
    return DESCONHECIDO


def descrever_maquina(repeticoes):
    data = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    return (f"data: {data}\n"
            f"cpu: {cpu()}\n"
            f"nucleos: {os.cpu_count() or DESCONHECIDO}\n"
            f"memoria: {memoria()}\n"
            f"so: {sistema()}\n"
            f"compilador: {compilador()}\n"
            f"flags: {flags()}\n"
            f"repeticoes: {repeticoes} (a 1ª é o aquecimento)\n")


if __name__ == "__main__":
    print(descrever_maquina(11), end="")
