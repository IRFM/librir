import logging
import os
from subprocess import PIPE, Popen, check_output, CalledProcessError

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


def get_pid_windows(app_name):
    final_list = []
    command = Popen(
        ["tasklist", "/FI", f"IMAGENAME eq {app_name}", "/fo", "CSV"],
        stdout=PIPE,
        shell=False,
    )
    msg = command.communicate()
    output = str(msg[0])
    if "INFO" not in output:
        output_list = output.split(app_name)
        for i in range(1, len(output_list)):
            j = int(output_list[i].replace('"', "")[1:].split(",")[0])
            if j not in final_list:
                final_list.append(j)

    return final_list


def is_thermavip_opened():
    names = ["Thermavip.exe", "Thermavip", "thermavip"]
    processes = []
    for n in names:
        try:
            processes = get_pid_of(n)
        except CalledProcessError:
            continue
        if processes:
            break
    return any(processes)


def get_pid_of(name):
    strategy = {"nt": get_pid_windows, "posix": get_pid_unix}
    return strategy[os.name](name)


def get_pid_unix(name):
    return check_output(["pidof", name])


def init_thermavip(th_instance="Thermavip-1"):
    if not is_thermavip_opened():
        logger.error("Thermavip couldn't be found")
        return

    try:
        import Thermavip as th
    except ImportError as e:
        logger.error("Thermavip Python module couldn't be imported")
        logger.error(e)
    else:
        th.setSharedMemoryName(th_instance)
        return th


def unbind_thermavip_shared_mem(th):
    if th is not None:
        th._SharedMemory.thread.stopth = True


def thermavip(func, *args, **kwargs):
    def wrapper(*args, **kwargs):
        th = init_thermavip()
        func(*args, **kwargs)
        unbind_thermavip_shared_mem(th)

    return wrapper
