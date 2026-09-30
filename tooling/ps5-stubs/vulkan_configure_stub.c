/*
 * PS5 RetroArch - Vulkan configure stub.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Configure-time link declaration for -lvulkan; never linked into the title,
 * never packaged or executed.
 *
 * RetroArch's configure, with --enable-vulkan, links a test program against
 * -lvulkan and stops when that fails (qb/config.libs.sh: check_lib '' VULKAN
 * -lvulkan vkCreateInstance). Nothing on the PS5 answers that probe: the SDK
 * ships no libvulkan, and ../PS5_Vulkan's drivers are archives that
 * tools/build-title.sh links into the title directly (libvulkan_radeon.ps5.a
 * for RADV, libps5vk.ps5.a for ps5vk), under names -lvulkan does not find.
 * The probe's only effect is HAVE_VULKAN in config.h; the -lvulkan it records
 * in config.mk is never read by this project's link.
 *
 * tools/retroarch-sources.sh builds this with configure's own compiler into
 * build/configure-stubs/libvulkan.a and appends that directory to configure's
 * LDFLAGS alone; the title's link (tools/build.sh) never names it.
 */

/* The probe declares `void vkCreateInstance(void);` and calls it; only the
 * symbol matters. VK_ERROR_INITIALIZATION_FAILED, were it ever called. */
int vkCreateInstance(void) { return -3; }
