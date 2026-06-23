#ifndef APP_POWER_CTRL_H
#define APP_POWER_CTRL_H

/* Return 1 if the rail was physically enabled, 0 if it was already on. */
int power_ctrl_vperiph_on(void);
int power_ctrl_vperiph_on_for_i2c1(void);
int power_ctrl_vperiph_on_for_i2c2(void);
int power_ctrl_vperiph_off(void);
int power_ctrl_gps_on(void);
int power_ctrl_gps_off(void);
int power_ctrl_prepare_suspend(void);
int power_ctrl_prepare_deep_sleep_all(void);

#endif /* APP_POWER_CTRL_H */
