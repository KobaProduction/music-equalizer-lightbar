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
        '#define DEF_MODEL_NUMBER_STR\t\t"ELK-BLEDOM-MELB"',
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
    header_path = output / "source" / "thb2_main.h"
    header_text = header_path.read_text(encoding="utf-8")
    header_text = replace_once(
        header_text,
        "#define LCD_TIMER_EVT         0x0400  // Timer related to display sleep and key long press feature expired",
        "#define LCD_TIMER_EVT         0x0400  // Timer related to display sleep and key long press feature expired\n#define MELB_LOCAL_CONTROL_EVT  0x0800  // Music-Light-V3 local buttons/animation tick",
        "reserve local-control OSAL event",
    )
    header_path.write_text(header_text, encoding="utf-8")

    main_text = replace_once(
        main_text,
        '#include "sbp_profile.h"',
        '#include "sbp_profile.h"\n\nextern void melb_lotus_local_init(void);\nextern void melb_lotus_local_tick(void);',
        "declare local-control hooks",
    )
    main_text = replace_once(
        main_text,
        "SimpleProfile_AddService( GATT_ALL_SERVICES );\t\t//\tSimple GATT Profile",
        "SimpleProfile_AddService( GATT_ALL_SERVICES );\t\t//\tSimple GATT Profile\n\tmelb_lotus_local_init();\n\tosal_start_reload_timer(simpleBLEPeripheral_TaskID, MELB_LOCAL_CONTROL_EVT, 10);",
        "start local-control timer",
    )
    main_text = replace_once(
        main_text,
        "\tif(events & SBP_CMDDATA) {",
        "\tif(events & MELB_LOCAL_CONTROL_EVT) {\n\t\tmelb_lotus_local_tick();\n\t\treturn(events ^ MELB_LOCAL_CONTROL_EVT);\n\t}\n\tif(events & SBP_CMDDATA) {",
        "local-control event handler",
    )
    main_path.write_text(main_text, encoding="utf-8")

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
        "lotus_lantern.c",
        "lotus_lantern.h",
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
        project / "src" / "pvvx_lotus_sbp_profile.c",
        output / "source" / "sbp_profile.c",
    )

    makefile_path = output / "Makefile"
    makefile = makefile_path.read_text(encoding="utf-8")
    marker = "SRC_PRJ += sbp_profile.c\n"
    additions = (
        marker
        + "SRC_PRJ += ble_control.c\n"
        + "SRC_PRJ += lotus_lantern.c\n"
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
