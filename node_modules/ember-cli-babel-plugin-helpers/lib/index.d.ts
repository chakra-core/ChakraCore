/**
 * A Babel plugin is typically specified as a string representing a module to be
 * loaded, either a package name or a full path. It's also possible for the actual
 * implementation to be supplied, in which case we check the value of a `name` field.
 */
export declare type BabelPlugin = string | {
    name?: string;
};
/**
 * Configuration for a Babel plugin may be the bare plugin itself, or a tuple
 * optionally containing configuration for the plugin and a possible unique
 * identifier to allow for multiple instances of the same plugin.
 */
export declare type BabelPluginConfig = BabelPlugin | [BabelPlugin, unknown?, unknown?];
/**
 * A configuration target is typically an `EmberApp` or `Addon` instance, which
 * may already have plugins configured or other options set.
 */
export interface ConfigurationTarget {
    options?: {
        babel?: {
            plugins?: BabelPluginConfig[];
        };
    };
}
/**
 * Locates the existing configuration, if any, for a given plugin.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance whose configuration should be checked
 * @param plugin The name of the plugin to be located
 */
export declare function findPlugin(config: ConfigurationTarget | BabelPluginConfig[], plugin: string): BabelPluginConfig | undefined;
/**
 * Indicates whether the given plugin is already present in the target's configuration.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance whose configuration should be checked
 * @param plugin The name of the plugin to be located
 */
export declare function hasPlugin(config: BabelPluginConfig[] | ConfigurationTarget, plugin: string): boolean;
export interface AddPluginOptions {
    /**
     * Any plugins that the given one must appear *before* in the plugins array.
     */
    before?: string[];
    /**
     * Any plugins that the given one must appear *after* in the plugins array.
     */
    after?: string[];
}
/**
 * Add a plugin to the Babel configuration for the given target.
 *
 * @param config An array of plugin configuration or an `EmberApp` or `Addon`
 *   instance for which the plugin should be set up
 * @param plugin Configuration for the Babel plugin to add
 * @param options Optional constraints around where the plugin should appear in the array
 */
export declare function addPlugin(config: ConfigurationTarget | BabelPluginConfig[], plugin: BabelPluginConfig, options?: AddPluginOptions): void;
