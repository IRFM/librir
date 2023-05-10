import pytest
import numpy as np
from librir import WESTIRMovie

from librir.registration.compute_registration import (
    correct_view,
    modify_img,
    modify_img_divup,
    modify_img_divdown,
    modify_img_wa,
)

#######################

MOV_LH = WESTIRMovie(56927, "LH1")


@pytest.mark.parametrize("img", MOV_LH.load_pos(2, 1))
def test_modify_img(img):
    assert img[0:512, :] != modify_img(img)


def test_correct_view():
    assert correct_view(55567, "ICRQ1B") == "ICR2Q1B"
    assert correct_view(55567, "ICRQ2B") == "ICR1Q2B"
    assert correct_view(55567, "ICRQ4A") == "ICR3Q4A"
    assert correct_view(56927, "DIVQ1B") == "DIVQ1B"
