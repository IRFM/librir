"""
Ce fichier est le fichier qui permet d'avoir les estimations du deplacement
des pixels par l'algorithme masked_registrator_ecc.
"""

###########################

import os
import numpy as np
import pandas as pd
import cv2
import librir

# from librir.west.WESTIRMovie import WESTIRMovie
from .masked_registration_ecc import MaskedRegistratorECC

###########################


def manage_computation_and_tries(img, regis_obj):
    """
    Fonction qui calcule les deplacements en x, en y et le niveau de
    confiance. Si l'algorithme ne parvient pas a converger pour une image,
    il essaie 4 autres fois en baissant la mediane a chaque essai de 0.01.
    Si au bout de 5 essais, il n'a toujours pas reussit a converger, il prend
    le deplacement en x, en y et le niveau de confiance de l'image precedente.
    """
    nb_try = 0
    max_try = 5
    compute = False

    while nb_try < max_try and not compute:
        try:
            regis_obj.compute(img)
            compute = True
            if regis_obj.check_median_value(1):
                regis_obj.define_median_value(1)
        except cv2.error:
            regis_obj.decrease_median(0.01)
            nb_try += 1
        if nb_try > 0:
            print("try number : {}".format(nb_try))

    if nb_try >= max_try:
        regis_obj.append_last_coordinates_and_confidence()
        print("took previous estimates.")

    return regis_obj


def compute_confidence_and_x_y_trajectories(
    pulse_obj, view, lower_bound, upper_bound, calibration, *args
):
    """
    Fonction qui permet d'estimer les deplacements (x,y) des pixels
    par rapport a une image de reference pour tout un choc.
    """
    if view.startswith("DIV"):
        is_div = True
    else:
        is_div = False

    for img_number in range(lower_bound, upper_bound + 1):

        if img_number % 50 == 0:
            print(img_number)
        img = pulse_obj.load_pos(img_number, calibration)

        if is_div:
            regis_obj_up = manage_computation_and_tries(img, args[0])
            regis_obj_down = manage_computation_and_tries(img, args[1])
        else:
            regis_obj = manage_computation_and_tries(img, args[0])

    if is_div:
        return regis_obj_up, regis_obj_down
    return regis_obj


def correct_view(nb_pulse, view):
    """
    Fonction qui permet de donner le bon nom a la visee qui se
    situe en dessous d'un certain numero de choc.
    """
    if nb_pulse > 55995:
        return view
    elif view == "ICRQ1B":
        return "ICR2Q1B"
    elif view == "ICRQ2B":
        return "ICR1Q2B"
    elif view == "ICRQ4A":
        return "ICR3Q4A"
    else:
        return view


def load_mask(view, path, modif):
    """
    Fonction qui permet de charger le masque associe
    a la visee actuellement etudiee.
    """
    if view == "WAQ5B":
        mask = np.loadtxt(os.path.join(path, "masks", "WAQ5B.txt"))
        #  mask = np.rot90(mask, k=3).copy()
    elif view == "DIVQ1B":
        mask = np.loadtxt(os.path.join(path, "masks", "DIVQ1B.txt"))
    elif view == "DIVQ6A":
        mask = np.loadtxt(os.path.join(path, "masks", "DIVQ6A.txt"))
    elif view == "DIVQ4A":
        mask = np.loadtxt(os.path.join(path, "masks", "DIVQ4A.txt"))
    elif view == "DIVQ2B":
        mask = np.loadtxt(os.path.join(path, "masks", "DIVQ2B.txt"))
    elif view == "DIVQ6B":
        mask = np.loadtxt(os.path.join(path, "masks", "DIVQ6B.txt"))
    elif view == "ICRQ1B":
        mask = np.loadtxt(os.path.join(path, "masks", "ICRQ1B.txt"))
    elif view == "ICRQ2B":
        mask = np.loadtxt(os.path.join(path, "masks", "ICRQ2B.txt"))
    elif view == "ICRQ4A":
        mask = np.loadtxt(os.path.join(path, "masks", "ICRQ4A.txt"))
    elif view == "LH1Q6A":
        mask = np.loadtxt(os.path.join(path, "masks", "LH1Q6A.txt"))
    elif view == "LH2Q6B":
        mask = np.loadtxt(os.path.join(path, "masks", "LH2Q6B.txt"))
    else:
        mask = np.loadtxt(os.path.join(path, "masks", "HRQ3B.txt"))

    mask = modif(np.array(mask, np.uint8)).copy()
    return mask


def modify_img_wa(img):
    """
    Fonction qui modifie l'image etudiee lorsque
    l'image est issue du grand angle.
    """
    return np.rot90(img[0:512, :]).copy()


def modify_img_divup(img):
    """
    Fonction qui modifie l'image etudiee lorsque
    l'image est issue du divertor haut.
    """
    return img[0 : int(512 / 2), :]


def modify_img_divdown(img):
    """
    Fonction qui modifie l'image etudiee lorsque
    l'image est issue du divertor bas.
    """
    return img[int(512 / 2) : 512, :]


def modify_img(img):
    """
    Fonction qui modifie l'image etudiee lorsque
    la visee est differente du divertor ou le grand angle.
    """
    return img[0:512, :]


def save_results(lower_bound, upper_bound, filename, *args):
    """
    Fonction qui permet de sauvegarder les listes contenant l'estimation
    du deplacement des pixels dans des fichiers au format .csv.
    """
    if len(args) > 1:
        tab_up = args[0].return_coordinates_and_confidence_values()
        tab_down = args[1].return_coordinates_and_confidence_values()

        concat = np.concatenate((tab_up, tab_down), axis=1)

        stab_data = pd.DataFrame(
            data=concat,
            index=np.arange(lower_bound, upper_bound + 1),
            columns=[
                "x-axis translations up",
                "y-axis translations up",
                "Confidence level up",
                "x axis translations down",
                "y axis translations down",
                "Confidence level down",
            ],
        )
    else:
        tab_other_view = args[0].return_coordinates_and_confidence_values()

        stab_data = pd.DataFrame(
            data=tab_other_view,
            index=np.arange(lower_bound, upper_bound + 1),
            columns=["x-axis translations", "y-axis translations", "Confidence level"],
        )

    stab_data.to_csv(filename, sep="\t")


def cam_to_file(cam, viewname, lower_bound, path):
    """
    Fonction qui permet, a partir d'un nom de camera
    de creer le fichier permettant de stabiliser un
    film IR de WEST
    """

    med = 1

    # open video
    cam.enable_bad_pixels = True

    upper_bound = cam.images - 1

    if viewname.startswith("DIV"):
        modify_up = modify_img_divup
        modify_down = modify_img_divdown

        mask_img_up = load_mask(viewname, os.path.dirname(librir.__file__), modify_up)
        mask_img_down = load_mask(
            viewname, os.path.dirname(librir.__file__), modify_down
        )

        regis_obj_up = MaskedRegistratorECC(
            window_factorh=1,
            window_factorv=1,
            sigma=0.5,
            mask=mask_img_up,
            median=med,
            pre_process=modify_up,
            view=viewname,
        )

        regis_obj_down = MaskedRegistratorECC(
            window_factorh=1,
            window_factorv=1,
            sigma=0.5,
            mask=mask_img_down,
            median=med,
            pre_process=modify_down,
            view=viewname,
        )

        regis_obj_up.start(cam.load_pos(lower_bound, 1))
        regis_obj_down.start(cam.load_pos(lower_bound, 1))

        regis_obj_up, regis_obj_down = compute_confidence_and_x_y_trajectories(
            cam, viewname, lower_bound + 1, upper_bound, 1, regis_obj_up, regis_obj_down
        )

        save_results(lower_bound, upper_bound, path, regis_obj_up, regis_obj_down)

        # Delete camera and registrator object
        del cam, regis_obj_up, regis_obj_down
    else:
        modify = modify_img

        mask_img = load_mask(viewname, os.path.dirname(librir.__file__), modify)

        regis_obj = MaskedRegistratorECC(
            window_factorh=1,
            window_factorv=1,
            sigma=0.5,
            mask=mask_img,
            median=med,
            pre_process=modify,
            view=viewname,
        )

        regis_obj.start(cam.load_pos(lower_bound, 1))

        regis_obj = compute_confidence_and_x_y_trajectories(
            cam, viewname, lower_bound + 1, upper_bound, 1, regis_obj
        )

        save_results(lower_bound, upper_bound, path, regis_obj)

        # Delete camera and masked_registrator_ECC object
        del cam, regis_obj


def compute_registration_ir(view_name, pulse_or_filename, outfile):
    """
    Fonction qui charge un film IR de WEST et appelle la fonction
    qui permet de sauvegarder le fichier de stabilisation.
    """
    import librir

    print(librir.__file__)
    import librir.west as west
    import librir.video_io as video

    view = view_name
    pulse = 0
    filename = str()
    try:
        pulse = int(pulse_or_filename)
    except ValueError:
        filename = pulse_or_filename
    outfilename = outfile

    # cam = None
    if pulse > 0:
        cam = west.WESTIRMovie(pulse, view)
    else:
        cam = video.IRMovie.from_filename(filename)

    cam_to_file(cam, view, 0, outfilename)


##########################


if __name__ == "__main__":

    # Arguments: view_name pulse_or_file out_file
    import sys

    if len(sys.argv) != 4:
        raise RuntimeError("wrong arguments (expected 3)")

    compute_registration_ir(str(sys.argv[1]), sys.argv[2], sys.argv[3])

    # PULSES = np.arange(55555, 55556)

    # CAMERA = 'HR'

    # FIRST_IMG_STUDIED = 0

    # PATH_TO_STABILISATION_FOLDER = r"path\to\your\stabilisation\folder"

    # from_pulses_to_csv_files(PULSES, CAMERA, FIRST_IMG_STUDIED, PATH_TO_STABILISATION_FOLDER)
