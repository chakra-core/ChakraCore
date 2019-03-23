export interface BrocfileOptions {
    /**
     * Environment setting. Default: development
     * This option is set by the --environment/--prod/--dev CLI argument.
     * This option can be used to conditionally affect a build pipeline in order to load different plugins for
     * development/production/testing
     */
    env: string;
}
