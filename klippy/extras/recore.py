# Config for Recore,
#
# Copyright (C) 2017-2019  Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
import logging, os
import pins, mcu

class recore:
    def __init__(self, config):
        printer = config.get_printer()
        ppins = printer.lookup_object('pins')
        ppins.register_chip('recore', self)
        revisions = {'A' + str(i): 'A' + str(i) for i in range(7)}
        self.revision = config.getchoice('revision', revisions)
        # Setup enable pin
        enable_pin = config.get('enable_pin', 'ar100:EN_HP')
        self.mcu_power_enable = ppins.setup_pin('digital_out', enable_pin)
        self.mcu_power_enable.setup_start_value(start_value=0.,
                                                shutdown_value=1.,
                                                is_static=False)
        self.mcu_power_enable.setup_max_duration(0.)

        if self.revision in ['A3', 'A4', 'A5', 'A6']:
            for idx in range(4):
                gain = config.get('gain_t' + str(idx), '1')
                if gain not in ['1', '100']:
                    raise Exception("Gain not 1 or 100")
                pin_name = "ar100:GAIN_ENABLE_T" + str(idx)
                if gain == '1':
                    pin = ppins.setup_pin('endstop', pin_name)
                else:
                    pin = ppins.setup_pin('digital_out', pin_name)
                    pin.setup_start_value(start_value=0.,
                                          shutdown_value=0.,
                                          is_static=True)

                pullup = config.get('pullup_t' + str(idx), '1')
                if pullup not in ['0', '1']:
                    raise Exception("Pullup not 0 or 1")
                pin_name = "ar100:PU_ENABLE_T" + str(idx)
                if pullup == '0':
                    pin = ppins.setup_pin('endstop', pin_name)
                else:
                    pin = ppins.setup_pin('digital_out', pin_name)
                    pin.setup_start_value(start_value=1.,
                                          shutdown_value=1.,
                                          is_static=True)


def load_config(config):
    return recore(config)