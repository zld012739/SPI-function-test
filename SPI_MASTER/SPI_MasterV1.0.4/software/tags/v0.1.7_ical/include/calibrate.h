/**	\file calibrate.h
 * 	\brief Header file for calibration algorithms.
 *
 *
 */


void CalibrateSensors(int16_t *rawReadings, uint16_t *gain, iq27 *scaledSensors);
void InitCalibration(void);
