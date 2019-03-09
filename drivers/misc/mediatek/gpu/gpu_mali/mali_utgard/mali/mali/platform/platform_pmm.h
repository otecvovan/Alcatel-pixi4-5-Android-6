
#ifndef __PLATFORM_PMM_H__
#define __PLATFORM_PMM_H__

struct mali_gpu_utilization_data;

typedef enum mali_power_mode
{
    MALI_POWER_MODE_ON  = 0x1,
    MALI_POWER_MODE_DEEP_SLEEP,
    MALI_POWER_MODE_LIGHT_SLEEP,
    //MALI_POWER_MODE_NUM
} mali_power_mode;

typedef enum mali_dvfs_action
{
    MALI_DVFS_CLOCK_UP = 0x1,
    MALI_DVFS_CLOCK_DOWN,
    MALI_DVFS_CLOCK_NOP
} mali_dvfs_action;

void mali_pmm_init(void);

void mali_pmm_deinit(void);

void mali_pmm_tri_mode(mali_power_mode mode);

void mali_pmm_utilization_handler(struct mali_gpu_utilization_data *data);
unsigned int gpu_get_current_utilization(void);

void mali_platform_power_mode_change(mali_power_mode power_mode);

#endif

