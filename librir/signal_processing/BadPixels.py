# -*- coding: utf-8 -*-
"""
Created on Wed Sep 16 12:59:54 2020

@author: VM213788
"""


from ..low_level.rir_signal_processing import (
    bad_pixels_create,
    bad_pixels_correct,
    bad_pixels_destroy,
)


class BadPixels:
    """
    Class used to correct bad pixels inside an IR handle
    """

    def __init__(self, first_image):
        self.handle = bad_pixels_create(first_image)

    def __del__(self):
        bad_pixels_destroy(self.handle)

    def correct(self, img):
        return bad_pixels_correct(self.handle, img)
