# Copyright (c) 2017, CNRS-LAAS
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import unittest

import fire_rs.firemodel.rothermel as rothermel


class TestRothermel(unittest.TestCase):

    def test_ros(self):
        for fuel, moisture, wind, slope, ros in validated_ros_by_conf:
            res = rothermel.ros(fuel, moisture, wind, slope)
            self.assertAlmostEqual(ros, res.ros)


# A set of of (fuel, moisture, wind[km/h], slope[percent], RoS[m/s])
# validated against the R implementation of Rothermel model
validated_ros_by_conf = [('A4', 'D3L3', 16.666666666666668, 33.333333333333329,
                          0.63882526322694466),
                         ('TU5', 'D1L2', 3.3333333333333335, 44.444444444444443,
                          0.020192522975768217),
                         ('GR7', 'D2L4', 0.0, 77.777777777777771, 2.2930427016296044e-05),
                         ('GR7', 'D2L4', 16.666666666666668, 33.333333333333329,
                          9.1409589091555313e-05),
                         ('SH9', 'D4L2', 20.0, 22.222222222222221, 0.54578617610635527),
                         ('A7', 'D2L3', 0.0, 55.555555555555557, 0.024616320277588734),
                         ('A7', 'D2L4', 23.333333333333336, 33.333333333333329,
                          0.23411638391864645),
                         ('GR3', 'D1L3', 3.3333333333333335, 0.0, 0.046043747283161927),
                         ('GR3', 'D4L1', 3.3333333333333335, 22.222222222222221,
                          0.0038569239030955362),
                         ('GR4', 'D3L1', 13.333333333333334, 55.555555555555557,
                          0.0014865980495879577),
                         ('A1', 'D3L3', 6.666666666666667, 0.0, 0.00033245919412971611),
                         ('SB1', 'D3L2', 13.333333333333334, 77.777777777777771,
                          0.012439244260481047),
                         ('GR2', 'D3L2', 20.0, 0.0, 0.36251613795627591),
                         ('GR2', 'D4L1', 20.0, 88.888888888888886, 0.00093230403515808457),
                         ('A3', 'D2L4', 26.666666666666668, 22.222222222222221,
                          0.0066185058774613946),
                         ('TL6', 'D4L3', 13.333333333333334, 33.333333333333329,
                          0.0012515506669130126),
                         ('A9', 'D3L2', 10.0, 11.111111111111111, 0.00025704589360155063),
                         ('A9', 'D4L4', 13.333333333333334, 33.333333333333329,
                          0.00039241845410956994),
                         ('A9', 'D3L1', 3.3333333333333335, 66.666666666666657,
                          0.00027973298651448299),
                         ('A13', 'D4L3', 6.666666666666667, 11.111111111111111,
                          0.010999292900895411),
                         ('A6', 'D4L1', 0.0, 44.444444444444443, 0.0053612235644453759),
                         ('TU2', 'D2L2', 6.666666666666667, 100.0, 0.048269856209825927),
                         ('TU2', 'D2L3', 30.0, 100.0, 0.14564069869557075),
                         ('A10', 'D3L1', 16.666666666666668, 44.444444444444443,
                          0.064773636748246086),
                         ('GR8', 'D4L4', 3.3333333333333335, 55.555555555555557,
                          0.0013080379979266451),
                         ('SH1', 'D1L1', 23.333333333333336, 100.0, 0.44703574156896769),
                         ('SH1', 'D4L4', 0.0, 88.888888888888886, 0.00028213371218010108),
                         ('TL5', 'D4L3', 26.666666666666668, 66.666666666666657,
                          0.011870509603888342),
                         ('TL5', 'D4L1', 6.666666666666667, 33.333333333333329,
                          0.0021275629660126106),
                         ('A12', 'D3L3', 26.666666666666668, 44.444444444444443,
                          0.046860149598947451)]
