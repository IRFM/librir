# -*- coding: utf-8 -*-
"""
Created on Fri Mar 26 14:39:16 2021

@author: CS266247
"""

import numpy as np
import librir as lr
import librir.signal_processing
import cv2

########################


class MaskedRegistratorECC:
    """
    Compute sub-pixels translations in a movie based on the OpenCV findTransformECC function.

    This class uses a "smart" way to update the reference image from which translations are computed.
    Basically, the reference image is updated when the error value returned by findTransformECC
    is less than a certain threshold : min(Confidences) - 2*std(Confidences)

    To maximize the chances to get a good translation estimation, the input images can be cropped based
    on the window_factor value (that takes a centered sub window of input image) and sigma (to apply
    a gaussian filter on input image).
    
    Furtheremore, the computation can be performed on static mask only (boolean image) or on a dynamic mask that only contains the X% lowest pixels.
    
    The first image must be processed with MaskedRegistratorECC.start() function, the remaining ones with MaskedRegistratorECC.compute().
    The results are stored in MaskedRegistratorECC.x and MaskedRegistratorECC.y (lists of translations). The confidence on the computed translations is accessible in MaskedRegistratorECC.confidences.
    
    Note that the returned translation values correspond to the translation from the very first image passed to MaskedRegistratorECC.start().
    """

    def __init__(
        self,
        window_factorh=0.7,
        window_factorv=0.7,
        sigma=0.5,
        mask=None,
        median=1,
        ref=None,
        pre_process=None,
        view=None,
    ):
        """
        Initialize the registrator object.
        Parameters:
            - window_factorh compute registration on a sub-part of the image only (crop taking horizontally (window_factorh*100) percent of the image centered)
            - window_factorv compute registration on a sub-part of the image only (crop taking vertically (window_factorv*100) percent of the image centered)
            - sigma: apply a gaussian filter with given sigma (0 to disable) to input images. This might help the computation process by removing some noise.
            - mask: static mask (image of 0 and 1) to only compute the registration on some parts of the image. This allows (for instance) to remove the vignetting part (borders) from the computation.
            - median: dynamic mask. For each new image, only consider the (median*100) percent of the lowest pixels (ignoring highest values).
        """
        self.sigma = sigma
        self.x = []
        self.y = []
        self.confidences = []
        self.ref_img = None
        self.ref = ref
        if ref is not None and pre_process is not None:
            self.ref = pre_process(ref)
        if sigma > 0 and self.ref is not None:
            self.ref = lr.signal_processing.gaussian_filter(self.ref, sigma)
        self.mask_ref_img = None
        self.window_factorH = window_factorh
        self.window_factorV = window_factorv
        shape = (512, 640)
        self.subW = int(shape[1] * self.window_factorH)
        self.subH = int(shape[0] * self.window_factorV)
        self.startX = int((shape[1] - self.subW) / 2)
        self.startY = int((shape[0] - self.subH) / 2)
        self.mask = mask
        self.conf_thresh = None
        self.pre_process = pre_process
        self.view = view
        self.median = median
        self.start_mat = np.eye(2, 3, dtype=np.float32)

    def start(self, img):
        if self.pre_process is not None:
            img = self.pre_process(img)
        if self.sigma > 0:
            img = lr.signal_processing.gaussian_filter(img, self.sigma)

        self.ref_img = img[
            self.startY : self.startY + self.subH, self.startX : self.startX + self.subW
        ]

        if self.mask is not None:
            self.mask = self.mask[
                self.startY : self.startY + self.subH,
                self.startX : self.startX + self.subW,
            ]

        self.x.append(0)
        self.y.append(0)
        self.confidences.append(1)

    def compute(self, img):
        if self.pre_process is not None:
            img = self.pre_process(img)
        if self.sigma > 0:
            img = lr.signal_processing.gaussian_filter(img, self.sigma)
        new_im = img[
            self.startY : self.startY + self.subH, self.startX : self.startX + self.subW
        ].copy()

        # TODO : Maybe include euclidean motion for Divertor view
        # Define the motion model - euclidean is rigid (SRT)
        # if self.view.startswith('DIV'):
        #   warp_mode = cv2.MOTION_EUCLIDEAN
        # else:
        warp_mode = cv2.MOTION_TRANSLATION

        # Specify the number of iterations.
        number_of_iterations = 500

        # Specify the threshold of the increment
        # in the correlation coefficient between two iterations
        termination_eps = 1e-3

        # Define termination criteria
        criteria = (
            cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT,
            number_of_iterations,
            termination_eps,
        )

        # TODO: do not necessarly make a copy, as the array could already be a float array due to gaussian filter
        if self.ref is None:
            im1 = np.array(self.ref_img, dtype=np.float32)
        else:
            im1 = np.array(self.ref, dtype=np.float32)
        im2 = np.array(new_im, dtype=np.float32)

        mask = None
        if self.mask is not None:
            mask = self.mask

        if self.median < 1:

            thresh1 = lr.signal_processing.find_median_pixel(new_im, self.median, mask)
            thresh2 = lr.signal_processing.find_median_pixel(
                self.ref_img, self.median, mask
            )
            thresh = max(thresh1, thresh2)

            m = (im1 > thresh) | (im2 > thresh)
            pixs = np.where(m)
            im1[pixs] = thresh
            im2[pixs] = thresh

        # normalize
        mi = np.min(im1)
        ma = np.max(im1)
        im1 = (im1 - mi) / (ma - mi)

        mi = np.min(im2)
        ma = np.max(im2)
        im2 = (im2 - mi) / (ma - mi)

        # Run the ECC algorithm. The results are stored in warp_matrix.
        (cc, warp_matrix) = cv2.findTransformECC(
            im1, im2, self.start_mat, warp_mode, criteria, mask, 1
        )
        self.start_mat = warp_matrix

        shift = [warp_matrix[1, 2], warp_matrix[0, 2]]
        confidence = cc

        self.confidences.append(confidence)

        self.x.append(shift[1])
        self.y.append(shift[0])

        if len(self.confidences) > 20 and self.ref is None:
            if self.conf_thresh is None:
                self.conf_thresh = np.min(self.confidences) - 2 * np.std(
                    self.confidences
                )
                print("confidence:", self.conf_thresh)
            if confidence < self.conf_thresh:
                print("reset at confidence ", len(self.x), confidence)
                new_im = lr.signal_processing.translate(new_im, -shift[1], -shift[0])
                self.ref_img = new_im
                self.start_mat = np.eye(2, 3, dtype=np.float32)

        return shift

    def decrease_median(self, value_of_decrease):
        self.median -= value_of_decrease
        return self.median

    def check_median_value(self, upper_value):
        if self.median < upper_value:
            return True
        else:
            return False

    def define_median_value(self, defined_value):
        self.median = defined_value
        return self.median

    def append_last_coordinates_and_confidence(self):
        self.x.append(self.x[-1])
        self.y.append(self.y[-1])
        self.confidences.append(self.confidences[-1])

    def return_coordinates_and_confidence_values(self):
        return np.array([self.x, self.y, self.confidences]).T
