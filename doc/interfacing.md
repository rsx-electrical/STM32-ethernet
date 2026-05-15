name = MEASURE_V_CMD
cmd = 1
response = "%4d,%4d,%4d,%4d", adc_measure.mv_12v, adc_measure.mv_24v, adc_measure.mv_55v, adc_measure.mv_batt_adc  
info = measures 12V, 24V, 55V and battery in mv  
<br>
name = MEASURE_B_CMD
cmd = 2
response =     	   "%4d,%4d,%4d,%4d,%4d,%4d",
    			   batt_voltage_mv[0], batt_voltage_mv[1], batt_voltage_mv[2], batt_voltage_mv[3],
				   adc_measure.mv_batt_adc, adc_measure.mv_batt_bms
info = batt_voltage_mv[x] are individual cell voltages, max 4200mV. <br>
adc_measure.mv_batt_adc is total battery voltage measured by an ADC <br>
adc_measure.mv_batt_bms is total battery voltage from the sum of batt_voltage_mv[x]  
<br>
name = MEASURE_A_CMD
cmd = 3
response = "%.2f,%.2f,%.2f", adc_measure.a_arm_motor, adc_measure.a_charger, adc_measure.a_batt  
info = measures arm+motor current, charger current (not available), and battery current (which should just =arm+motor current for now)  
<br>
name = MOTOR_ON_CMD
cmd = 4
response = motor on
<br>
name = MOTOR_OFF_CMD
cmd = 5
response = motor off
<br>
name = ARM_ON_CMD
cmd = 6
response = arm on
<br>
name = ARM_OFF_CMD
cmd = 7
response = arm off
<br>
name = ON_5V
cmd = 8
response = 5V on
<br>
name = ON_12V
cmd = 9
response = 12V on
<br>
name = ON_24V
cmd = 10
response = 24V on
<br>
name = ON_55V
cmd = 11
response = 55V on
<br>
name = OFF_5V
cmd = 12
response = 5V off
<br>
name = OFF_12V
cmd = 13
response = 12V off
<br>
name = OFF_24V
cmd = 14
response = 24V off
<br>
name = OFF_55V
cmd = 15
response = 55V off
<br>
name = ESTOP_CMD
cmd = 16  
response = N/A  
info = turns off rover  
<br>
name = R_LED_TOGGLE
cmd = 17
response = Red LED On/Off
<br>
name = G_LED_TOGGLE
cmd = 18
response = Green LED On/Off
<br>
name = B_LED_TOGGLE
cmd = 19
response = Blue LED On/Off