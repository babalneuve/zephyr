# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NXP_INPUTMUX nxp_inputmux.c)