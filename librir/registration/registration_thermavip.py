if __name__ == "__main__":

    import sys

    sys.path.insert(0, sys.argv[1])
    import librir
    import os

    print(sys.path)
    print(os.path.abspath(librir.__file__))
    print(os.path.abspath(librir.__file__))
    import librir.west
    import librir.registration
    from librir.registration.compute_registration import compute_registration_ir

    # Arguments: view_name pulse_or_file out_file
    import sys

    if len(sys.argv) != 5:
        raise RuntimeError("wrong arguments (expected 3)")

    compute_registration_ir(str(sys.argv[2]), sys.argv[3], sys.argv[4])
