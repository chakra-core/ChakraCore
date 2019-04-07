"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const plugin_names_1 = require("./-private/plugin-names");
const plugin_configuration_1 = require("./-private/plugin-configuration");
/**
 * Locates the existing configuration, if any, for a given plugin.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance whose configuration should be checked
 * @param plugin The name of the plugin to be located
 */
function findPlugin(config, plugin) {
    let plugins = plugin_configuration_1.getPluginsArray(config);
    let index = plugin_configuration_1.findPluginIndex(plugins, plugin);
    return plugins[index];
}
exports.findPlugin = findPlugin;
/**
 * Indicates whether the given plugin is already present in the target's configuration.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance whose configuration should be checked
 * @param plugin The name of the plugin to be located
 */
function hasPlugin(config, plugin) {
    return !!findPlugin(config, plugin);
}
exports.hasPlugin = hasPlugin;
/**
 * Add a plugin to the Babel configuration for the given target.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance for which the plugin should be set up
 * @param plugin Configuration for the Babel plugin to add
 * @param options Optional constraints around where the plugin should appear in the array
 */
function addPlugin(config, plugin, options = {}) {
    let plugins = plugin_configuration_1.getPluginsArray(config);
    let earliest = Math.max(...findPluginIndices(plugins, options.after)) + 1;
    let latest = Math.min(...findPluginIndices(plugins, options.before));
    if (earliest > latest) {
        throw new Error(`Unable to satisfy placement constraints for Babel plugin ${plugin_names_1.resolvePluginName(plugin)}`);
    }
    let targetIndex = Number.isFinite(latest) ? latest : Number.isFinite(earliest) ? earliest : plugins.length;
    plugins.splice(targetIndex, 0, plugin);
}
exports.addPlugin = addPlugin;
function findPluginIndices(plugins, pluginNames = []) {
    let pluginIndices = pluginNames.map(name => (name ? plugin_configuration_1.findPluginIndex(plugins, name) : -1));
    return pluginIndices.filter(index => index >= 0);
}
