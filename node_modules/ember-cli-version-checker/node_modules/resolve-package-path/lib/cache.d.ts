declare const _default: {
    new (): {
        _store: {
            [key: string]: string;
        };
        set(key: string, value: any): any;
        get(key: string): string;
        has(key: string): boolean;
        delete(key: string): void;
        readonly size: number;
    };
};
export = _default;
