# Support for voltage sensor wich checks often
#
# Copyright (C) 2021  Elias Bakken
#
# This file may be distributed under the terms of the GNU GPLv3 license.

import logging
ADC_REPORT_TIME = 0.001
ADC_SAMPLE_TIME = 0.0001
ADC_SAMPLE_COUNT = 10


class PrinterSensorVoltage:
    def __init__(self, config):
        self.printer = config.get_printer()
        self.name = config.get_name().split()[-1]
        pheaters = self.printer.load_object(config, 'heaters')
        self.sensor = pheaters.setup_sensor(config)
        self.factor = config.getfloat('factor', 1.0, minval=0.0)
        self.min_voltage = config.getfloat('min_voltage', 0.0,
                                           minval=0.0) / self.factor
        self.max_voltage = config.getfloat(
            'max_voltage', 99999999.9, above=self.min_voltage) / self.factor
        self.sensor.mcu_adc.setup_adc_callback(ADC_REPORT_TIME,
                                               self.voltage_callback)
        self.sensor.mcu_adc.setup_minmax(ADC_SAMPLE_TIME,
                                         ADC_SAMPLE_COUNT,
                                         self.min_voltage,
                                         self.max_voltage,
                                         range_check_count=1)
        self.sensor.setup_callback(self.voltage_callback)
        pheaters.register_sensor(config, self)
        self.last_voltage = 0.
        self.measured_min = 99999999.
        self.measured_max = 0.
        self.mcu = self.sensor.mcu_adc.get_mcu()
        self.mcu.register_config_callback(self._config_callback)

        self.printer.register_event_handler("idle_timeout:printing",
                                            self._handle_printing)
        self.printer.register_event_handler("idle_timeout:ready",
                                            self._handle_ready)

    def _handle_printing(self, print_time):
        logging.info("Handle printing {}".format(print_time))
        self.mcu.add_config_cmd("clear_position oid=%d" % (self.clear_oid))
        self.mcu.add_config_cmd("set_position oid=%d x=%u y=%u z=%u ts=%u" %
                                (self.set_oid, 5, 6, 7, 4))
        self.mcu.add_config_cmd("get_position oid=%d" % (self.get_oid))

    def _handle_ready(self, print_time):
        logging.info("Handle ready {}".format(print_time))

    def _config_callback(self):
        self.set_oid = self.mcu.create_oid()
        self.get_oid = self.mcu.create_oid()
        self.clear_oid = self.mcu.create_oid()
        self.mcu.add_config_cmd("get_position oid=%d" % (self.get_oid))
        self.mcu.register_response(self._handle_get_position, "position_state",
                                   self.get_oid)

    def _handle_get_position(self, params):
        x = params['x']
        y = params['y']
        z = params['z'] 
        logging.info("handle_get_position {}, {}, {}".format(x, y, z))

    def voltage_callback(self, read_time, voltage):
        voltage = voltage * self.factor
        self.last_voltage = voltage
        if voltage:
            self.measured_min = min(self.measured_min, voltage)
            self.measured_max = max(self.measured_max, voltage)

    def get_temp(self, eventtime):
        return self.last_voltage, 0.

    def stats(self, eventtime):
        return False, '%s: volt=%.1f' % (self.name, self.last_voltage)

    def get_status(self, eventtime):
        return {
            'voltage': self.last_voltage,
            'measured_min_voltage': self.measured_min,
            'measured_max_voltage': self.measured_max
        }


def load_config_prefix(config):
    return PrinterSensorVoltage(config)
