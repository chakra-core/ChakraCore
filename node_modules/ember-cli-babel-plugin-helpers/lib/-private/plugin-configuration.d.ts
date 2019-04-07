import { ConfigurationTarget, BabelPluginConfig } from '..';
export declare function getPluginsArray(target: ConfigurationTarget | BabelPluginConfig[]): BabelPluginConfig[];
export declare function findPluginIndex(plugins: BabelPluginConfig[], plugin: string): number;
