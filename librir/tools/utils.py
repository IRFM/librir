import logging
import os
from subprocess import PIPE, Popen, check_output


import functools
import inspect
import warnings

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

string_types = (type(b""), type(""))


def deprecated(reason):
    """
    This is a decorator which can be used to mark functions
    as deprecated. It will result in a warning being emitted
    when the function is used.
    """

    if isinstance(reason, string_types):

        # The @deprecated is used with a 'reason'.
        #
        # .. code-block:: python
        #
        #    @deprecated("please, use another function")
        #    def old_function(x, y):
        #      pass

        def decorator(func1):

            if inspect.isclass(func1):
                fmt1 = "Call to deprecated class {name} ({reason})."
            else:
                fmt1 = "Call to deprecated function {name} ({reason})."

            @functools.wraps(func1)
            def new_func1(*args, **kwargs):
                warnings.simplefilter("always", DeprecationWarning)
                warnings.warn(
                    fmt1.format(name=func1.__name__, reason=reason),
                    category=DeprecationWarning,
                    stacklevel=2,
                )
                warnings.simplefilter("default", DeprecationWarning)
                return func1(*args, **kwargs)

            return new_func1

        return decorator

    elif inspect.isclass(reason) or inspect.isfunction(reason):

        # The @deprecated is used without any 'reason'.
        #
        # .. code-block:: python
        #
        #    @deprecated
        #    def old_function(x, y):
        #      pass

        func2 = reason

        if inspect.isclass(func2):
            fmt2 = "Call to deprecated class {name}."
        else:
            fmt2 = "Call to deprecated function {name}."

        @functools.wraps(func2)
        def new_func2(*args, **kwargs):
            warnings.simplefilter("always", DeprecationWarning)
            warnings.warn(
                fmt2.format(name=func2.__name__),
                category=DeprecationWarning,
                stacklevel=2,
            )
            warnings.simplefilter("default", DeprecationWarning)
            return func2(*args, **kwargs)

        return new_func2

    else:
        raise TypeError(repr(type(reason)))


def is_thermavip_opened():
    names = ["Thermavip.exe", "Thermavip", "thermavip"]
    processes = []
    for n in names:
        processes = get_pid_of(n)
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
        logger.error(f"Thermavip Python module couldn't be imported")
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


def remove_quotes(original: dict):
    d = original.copy()
    for key, value in d.items():
        if isinstance(value, str):
            d[key] = remove_quotes_string(d[key])

        if isinstance(value, dict):
            d[key] = remove_quotes(value)
    #
    return d


def remove_quotes_string(s):
    if s.startswith(('"', "'")):
        s = s[1:]
    if s.endswith(('"', "'")):
        s = s[:-1]
    return s


def clean_string(s):
    # todo: generalise
    data = remove_quotes_string(s)
    data = data.replace("°", "")
    if "," in data:
        logger.warning("all commas ',' are replaced with '.' for float casting")
        data = data.replace(",", ".")
    return data
