#!/usr/bin/env python3

import argparse
import pathlib
import shutil


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--thb2", required=True)
    parser.add_argument("--project", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    source = pathlib.Path(args.thb2) / "bthome_phy6222"
    project = pathlib.Path(args.project)
    output = pathlib.Path(args.output)

    if output.exists():
        shutil.rmtree(output)
    shutil.copytree(source, output)

    config_path = output / "source" / "config.h"
    config = config_path.read_text(encoding="utf-8")
    key2_start = config.index("#elif DEVICE == DEVICE_KEY2")
    key2_end = config.index("#elif DEVICE == DEVICE_TH04", key2_start)
    key2 = config[key2_start:key2_end]

    services_start = key2.index("#if OTA_TYPE == OTA_TYPE_BOOT")
    services_end = key2.index("#define ADC_PIN_USE_OUT", services_start)
    key2 = (
        key2[:services_start]
        + "#define DEV_SERVICES (OTA_TYPE)\n\n"
        + key2[services_end:]
    )

    gpio_start = key2.index("#define GPIO_KEY")
    model_start = key2.index("#define DEF_MODEL_NUMBER_STR", gpio_start)
    key2 = key2[:gpio_start] + key2[model_start:]

    key2 = replace_once(
        key2,
        '#define DEF_MODEL_NUMBER_STR\t\t"KEY"',
        '#define DEF_MODEL_NUMBER_STR\t\t"Triones"',
        "KEY2 model name",
    )
    config = config[:key2_start] + key2 + config[key2_end:]
    config_path.write_text(config, encoding="utf-8")

    low_level_main_path = output / "source" / "main.c"
    low_level_main = low_level_main_path.read_text(encoding="utf-8")
    low_level_main = replace_once(
        low_level_main,
        "\thal_adc_init();",
        "\t/* ADC intentionally disabled for BLE-only bring-up. */",
        "disable ADC init",
    )

    io_start = low_level_main.index("#elif (DEVICE == DEVICE_KEY2)")
    io_end = low_level_main.index("#elif (DEVICE == DEVICE_TH04)", io_start)
    io_block = low_level_main[io_start:io_end]
    io_block = io_block.replace("GPIO_PULL_UP_S", "GPIO_FLOATING")
    io_block = io_block.replace("GPIO_PULL_UP", "GPIO_FLOATING")
    io_block = io_block.replace("GPIO_PULL_DOWN", "GPIO_FLOATING")
    low_level_main = (
        low_level_main[:io_start] + io_block + low_level_main[io_end:]
    )
    low_level_main_path.write_text(low_level_main, encoding="utf-8")

    main_path = output / "source" / "thb2_main.c"
    main_text = main_path.read_text(encoding="utf-8")
    main_text = replace_once(
        main_text,
        "int len = flash_read_cfg(&p[2], EEP_ID_DVN, GAP_DEVICE_NAME_LEN - 1);",
        "int len = -1;",
        "force default BLE name",
    )
    main_text = replace_once(
        main_text,
        "flash_write_cfg(NULL, EEP_ID_DVN, 0);",
        "(void)0;",
        "avoid name-setting flash write",
    )
    main_text = replace_once(
        main_text,
        "void SimpleBLEPeripheral_Init( uint8_t task_id )\n{\n\tsimpleBLEPeripheral_TaskID = task_id;",
        "void SimpleBLEPeripheral_Init( uint8_t task_id )\n{\n\tsimpleBLEPeripheral_TaskID = task_id;\n\tLOG(\"\\nMELB boot: Music-Light-V3-221101 / ST17H66B\\n\");\n\tLOG(\"MELB: UART0 P9/P10 115200\\n\");\n\tLOG(\"MELB: BLE init begin\\n\");",
        "add MELB UART startup banner",
    )
    main_text = replace_once(
        main_text,
        "\t\tset_mac();",
        "\t\tset_mac();\n\t\tgapRole_AdvertDataLen = 7;\n\t\tgapRole_AdvertData[0] = 2;\n\t\tgapRole_AdvertData[1] = GAP_ADTYPE_FLAGS;\n\t\tgapRole_AdvertData[2] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;\n\t\tgapRole_AdvertData[3] = 3;\n\t\tgapRole_AdvertData[4] = GAP_ADTYPE_16BIT_COMPLETE;\n\t\tgapRole_AdvertData[5] = LO_UINT16(HAPPY_LIGHTING_SERVICE_UUID16);\n\t\tgapRole_AdvertData[6] = HI_UINT16(HAPPY_LIGHTING_SERVICE_UUID16);\n\t\tLOG(\"MELB: BLE name=%s\\n\", &gapRole_ScanRspData[2]);\n\t\tLOG(\"MELB: adv interval units=%u\\n\", (unsigned)(cfg.advertising_interval * 100));\n\t\tLOG(\"MELB: adv data=%02x %02x %02x %02x %02x %02x %02x\\n\", gapRole_AdvertData[0], gapRole_AdvertData[1], gapRole_AdvertData[2], gapRole_AdvertData[3], gapRole_AdvertData[4], gapRole_AdvertData[5], gapRole_AdvertData[6]);",
        "log BLE identity",
    )
    main_text = replace_once(
        main_text,
        "\tif ( events & SBP_RESET_ADV_EVT ) {\n\t\tLOG(\"SBP_RESET_ADV_EVT\\n\");",
        "\tif ( events & SBP_RESET_ADV_EVT ) {\n\t\tLOG(\"MELB: SBP_RESET_ADV_EVT -> enable advertising\\n\");",
        "log advertising enable",
    )
    main_text = replace_once(
        main_text,
        "\tif ( events & SBP_START_DEVICE_EVT ) {\n\t\t// Start the Device\n\t\tVOID GAPRole_StartDevice( &simpleBLEPeripheral_PeripheralCBs );",
        "\tif ( events & SBP_START_DEVICE_EVT ) {\n\t\tLOG(\"MELB: GAPRole_StartDevice\\n\");\n\t\tVOID GAPRole_StartDevice( &simpleBLEPeripheral_PeripheralCBs );",
        "log GAP role start",
    )
    main_text = replace_once(
        main_text,
        "\tif ( events & ADV_BROADCAST_EVT) {\n\t\tadv_measure();\n\t\tLOG(\"advN%u\\n\", adv_wrk.meas_count);",
        "\tif ( events & ADV_BROADCAST_EVT) {\n\t\t++adv_wrk.meas_count;\n\t\t++melb_adv_total;\n\t\t++melb_adv_since_report;\n\t\tif (!melb_adv_first_logged) {\n\t\t\tmelb_adv_first_logged = 1;\n\t\t\tLOG(\"MELB: advertising first event\\n\");\n\t\t}",
        "replace per-event advertising logs with counters",
    )
    header_path = output / "source" / "thb2_main.h"
    header_text = header_path.read_text(encoding="utf-8")
    header_text = replace_once(
        header_text,
        "#define LCD_TIMER_EVT         0x0400  // Timer related to display sleep and key long press feature expired",
        "#define LCD_TIMER_EVT         0x0400  // Timer related to display sleep and key long press feature expired\n#define MELB_LOCAL_CONTROL_EVT  0x0800  // Music-Light-V3 local buttons/animation tick\n#define MELB_HEARTBEAT_EVT      0x1000  // 1 Hz runtime liveness diagnostic",
        "reserve local-control OSAL event",
    )
    header_path.write_text(header_text, encoding="utf-8")

    main_text = replace_once(
        main_text,
        '#include "sbp_profile.h"',
        '#include "sbp_profile.h"\n#include "happylighting.h"\n\nstatic uint32_t melb_adv_total = 0;\nstatic uint32_t melb_adv_since_report = 0;\nstatic uint8_t melb_adv_first_logged = 0;\n\nextern void melb_happylighting_local_init(void);\nextern void melb_happylighting_local_tick(void);',
        "declare local-control hooks",
    )
    main_text = replace_once(
        main_text,
        "SimpleProfile_AddService( GATT_ALL_SERVICES );\t\t//\tSimple GATT Profile",
        "LOG(\"MELB: register HappyLighting FFD5/FFD9/FFD4\\n\");\n\tSimpleProfile_AddService( GATT_ALL_SERVICES );\t\t//\tSimple GATT Profile\n\tmelb_happylighting_local_init();\n\tosal_start_reload_timer(simpleBLEPeripheral_TaskID, MELB_LOCAL_CONTROL_EVT, 10);\n\tosal_start_reload_timer(simpleBLEPeripheral_TaskID, MELB_HEARTBEAT_EVT, 1000);",
        "start local-control timer",
    )
    main_text = replace_once(
        main_text,
        "\tif(events & SBP_CMDDATA) {",
        "\tif(events & MELB_HEARTBEAT_EVT) {\n\t\tstatic uint32_t melb_heartbeat = 0;\n\t\t++melb_heartbeat;\n\t\tif ((melb_heartbeat % 10u) == 0u) {\n\t\t\tLOG(\"MELB: 10s stats heartbeat=%lu adv_sent=%lu adv_total=%lu gap=%u adv=%u\\n\", (unsigned long)melb_heartbeat, (unsigned long)melb_adv_since_report, (unsigned long)melb_adv_total, gapProfileState, gapRole_AdvEnabled);\n\t\t\tmelb_adv_since_report = 0;\n\t\t}\n\t\treturn(events ^ MELB_HEARTBEAT_EVT);\n\t}\n\tif(events & MELB_LOCAL_CONTROL_EVT) {\n\t\tmelb_happylighting_local_tick();\n\t\treturn(events ^ MELB_LOCAL_CONTROL_EVT);\n\t}\n\tif(events & SBP_CMDDATA) {",
        "local-control event handler",
    )
    main_path.write_text(main_text, encoding="utf-8")

    config_c_path = output / "source" / "config.c"
    config_c = config_c_path.read_text(encoding="utf-8")
    config_c = replace_once(
        config_c,
        "void load_eep_config(void) {\n\tif(!flash_supported_eep_ver(0, APP_VERSION)) {",
        "void load_eep_config(void) {\n\t/* MELB bring-up: ignore incompatible factory application config bytes. */\n\tmemcpy(&cfg, &def_cfg, sizeof(cfg));\n\tcfg.advertising_interval = 32; /* 32 * 62.5 ms = 2000 ms */\n\tcfg.connect_latency = 0;\n\tcfg.batt_interval = 60;\n\ttest_config();\n\treturn;\n#if 0\n\tif(!flash_supported_eep_ver(0, APP_VERSION)) {",
        "force safe RAM config defaults",
    )
    config_c = replace_once(
        config_c,
        "\ttest_config();\n}\n\nvoid save_config(void)",
        "\ttest_config();\n#endif\n}\n\nvoid save_config(void)",
        "close disabled persisted config block",
    )
    config_c_path.write_text(config_c, encoding="utf-8")

    battery_path = output / "source" / "battery.c"
    battery_path.write_text(
        '#include "battery.h"\n\n'
        'void batt_start_measure(void) {}\n'
        'void check_battery(void) {}\n',
        encoding="utf-8",
    )

    copies = [
        "ble_control.c",
        "ble_control.h",
        "happylighting.c",
        "happylighting.h",
        "local_controls.c",
        "local_controls.h",
        "st17h66b_spi1.c",
        "st17h66b_spi1.h",
        "ws2812b.c",
        "ws2812b.h",
        "ws2812b_spi.c",
        "ws2812b_spi.h",
    ]

    for name in copies:
        shutil.copy2(project / "src" / name, output / "source" / name)

    shutil.copy2(
        project / "src" / "pvvx_happylighting_sbp_profile.c",
        output / "source" / "sbp_profile.c",
    )

    makefile_path = output / "Makefile"
    makefile = makefile_path.read_text(encoding="utf-8")
    makefile = replace_once(
        makefile,
        "DEFINES += -DDEBUG_INFO=0",
        "DEFINES += -DDEBUG_INFO=1",
        "enable UART debug logging",
    )
    makefile = replace_once(
        makefile,
        "SRCS += $(SDK_PATH)/components/driver/clock/clock.c",
        "SRCS += $(SDK_PATH)/components/driver/clock/clock.c\nSRCS += $(SDK_PATH)/components/driver/dma/dma.c\nSRCS += $(SDK_PATH)/components/driver/spi/spi.c",
        "enable PHYplus DMA driver",
    )
    marker = "SRC_PRJ += sbp_profile.c\n"
    additions = (
        marker
        + "SRC_PRJ += ble_control.c\n"
        + "SRC_PRJ += happylighting.c\n"
        + "SRC_PRJ += local_controls.c\n"
        + "SRC_PRJ += st17h66b_spi1.c\n"
        + "SRC_PRJ += ws2812b.c\n"
        + "SRC_PRJ += ws2812b_spi.c\n"
    )
    makefile = replace_once(
        makefile,
        marker,
        additions,
        "Makefile project source insertion",
    )
    makefile_path.write_text(makefile, encoding="utf-8")


if __name__ == "__main__":
    main()
