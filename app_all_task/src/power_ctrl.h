#ifndef APP_POWER_CTRL_H
#define APP_POWER_CTRL_H

int power_ctrl_vperiph_on(void);
int power_ctrl_vperiph_off(void);
int power_ctrl_gps_on(void);
int power_ctrl_gps_off(void);
int power_ctrl_prepare_suspend(void);
int power_ctrl_prepare_deep_sleep_all(void);

#endif /* APP_POWER_CTRL_H */
