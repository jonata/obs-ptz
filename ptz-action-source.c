#include <obs-module.h>
#include <obs-frontend-api.h>
#include <callback/signal.h>
#include "ptz-action-source.h"
#include <stdio.h>

struct ptz_action_source_data {
	unsigned int camera;
	unsigned int action;
	unsigned int preset;
	obs_source_t *src;
};

static const char *ptz_action_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "PTZ Action";
}

static void ptz_action_source_update(void *data, obs_data_t *settings)
{
	struct ptz_action_source_data *context = data;

	context->camera = (unsigned int) obs_data_get_int(settings, "scene_camera");
	context->action = (unsigned int) obs_data_get_int(settings, "scene_action");
	context->preset = (unsigned int) obs_data_get_int(settings, "scene_preset");
}

static void ptz_action_source_handler(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(calldata);
}

static void ptz_action_source_fe_callback(enum obs_frontend_event event, void *data)
{
	struct ptz_action_source_data *context = data;

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		break;
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		obs_source_t *previewsrc = obs_frontend_get_current_preview_scene();
		if (!previewsrc)
			return;
		obs_scene_t *preview = obs_scene_from_source(previewsrc);
		obs_sceneitem_t *item = obs_scene_sceneitem_from_source(preview, context->src);
		if (item) {
			obs_sceneitem_release(item);
			ptz_preset_recall(context->camera, context->preset);
		}
		obs_scene_release(preview);
		break;
	}
}

static void *ptz_action_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct ptz_action_source_data *context = bzalloc(sizeof(struct ptz_action_source_data));

	context->src = source;
	ptz_action_source_update(context, settings);

	signal_handler_t *sh = obs_source_get_signal_handler(source);
	signal_handler_connect(sh, "hide", ptz_action_source_handler, context);
	signal_handler_connect(sh, "activate", ptz_action_source_handler, context);
	signal_handler_connect(sh, "deactivate", ptz_action_source_handler, context);

	obs_frontend_add_event_callback(ptz_action_source_fe_callback, context);

	return context;
}

static void ptz_action_source_destroy(void *data)
{
	bfree(data);
}

static bool ptz_action_source_device_changed_callback(obs_properties_t *props,
					obs_property_t *prop_camera, obs_data_t *settings)
{

	obs_property_t *prop_preset = obs_properties_get(props, "scene_preset");
	obs_property_list_clear(prop_preset);

	/* Find the camera config */
	int64_t camera = obs_data_get_int(settings, "scene_camera");
	obs_data_array_t *array = ptz_devices_get_config();
	obs_data_t *config = obs_data_array_item(array, camera);

	obs_data_array_t *preset_array = obs_data_get_array(config, "presets");
	if (preset_array) {
		for (int i = 0; i < obs_data_array_count(preset_array); i++) {
			obs_data_t *preset = obs_data_array_item(preset_array, i);
			obs_property_list_add_int(prop_preset, obs_data_get_string(preset, "name"),
						obs_data_get_int(preset, "id"));
			obs_data_release(preset);
		}
		obs_data_array_release(preset_array);
	}

	obs_data_release(config);
	obs_data_array_release(array);
	return true;
}

static obs_properties_t *ptz_action_source_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *prop_camera = obs_properties_add_list(props, "scene_camera", "Camera",
						OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_t *prop_action = obs_properties_add_list(props, "scene_action", "Action",
						OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_t *prop_preset = obs_properties_add_list(props, "scene_preset", "Preset",
						OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(prop_action, "Preset Recall", 0);

	/* Enumerate the cameras */
	obs_data_array_t *array = ptz_devices_get_config();
	for (int i = 0; i < obs_data_array_count(array); i++) {
		obs_data_t *config = obs_data_array_item(array, i);
		obs_property_list_add_int(prop_camera, obs_data_get_string(config, "name"), i);
		obs_data_release(config);
	}
	obs_data_array_release(array);

	obs_property_set_modified_callback(prop_camera, ptz_action_source_device_changed_callback);

	return props;
}

struct obs_source_info ptz_action_source = {
	.id = "ptz_action_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = ptz_action_source_get_name,
	.create = ptz_action_source_create,
	.destroy = ptz_action_source_destroy,
	.update = ptz_action_source_update,
	.get_properties = ptz_action_source_get_properties,
};

bool ptz_action_source_load(void)
{
	return obs_register_source(&ptz_action_source);
}

