
#ifndef PTZ_ACTION_SOURCE_H
#define PTZ_ACTION_SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

extern bool ptz_action_source_load(void);

extern obs_data_array_t *ptz_devices_get_config();
extern void ptz_preset_recall(unsigned int camera, unsigned int preset);

#ifdef __cplusplus
}
#endif

#endif /* PTZ_ACTION_SOURCE_H */
