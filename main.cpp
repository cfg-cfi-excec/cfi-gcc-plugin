#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <vector>

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"
#include <context.h>
#include <basic-block.h>
#include <rtl.h>
#include <tree-pass.h>
#include <tree.h>
#include <print-tree.h>

#include "gcc_plugin.h"
#include "./implementations/gcc_plugin_hcfi.h"
#include "./implementations/gcc_plugin_hafix.h"

// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible = 1;

static struct plugin_info cfi_plugin_info =
{ "1.0", "GCC Plugin for instrumenting code with CFI instructions" };

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
	if(!plugin_default_version_check(version, &gcc_version)) {
		std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR
			<< "." << GCCPLUGIN_VERSION_MINOR << "\n";
		return 1;
	}

	GCC_PLUGIN *gcc_plugin = NULL;

	for (int i = 0; i < plugin_info->argc; i++) {
		if (std::strcmp(plugin_info->argv[i].key, "cfi_implementation") == 0) {
			const char* implementation = plugin_info->argv[i].value;

			if (std::strcmp(implementation, "HCFI") == 0) {
						std::string implementation = plugin_info->argv[i].value;
				gcc_plugin = new GCC_PLUGIN_HCFI(g, plugin_info->argv, plugin_info->argc);
				break;
			} else if (std::strcmp(implementation, "HAFIX") == 0) {
						std::string implementation = plugin_info->argv[i].value;
				gcc_plugin = new GCC_PLUGIN_HAFIX(g, plugin_info->argv, plugin_info->argc);
				break;
			} else {
				std::cerr << "Invalid CFI Implementation declared (" << implementation << ")\n";
				return 1;
			}

			break;
		}
	}

	register_callback(plugin_info->base_name,
					/* event */ PLUGIN_INFO,
					/* callback */ NULL,
					/* user_data */
					&cfi_plugin_info);

	// Register the phase right after cfg
	struct register_pass_info pass_info;

	pass_info.pass = gcc_plugin;
	pass_info.reference_pass_name = "*free_cfg";
	pass_info.ref_pass_instance_number = 1;
	pass_info.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

	return 0;
}