import { IMinimatch } from 'minimatch';
declare const _default: {
    new (matchers: (string | IMinimatch)[]): {
        matchers: IMinimatch[];
        match(value: string): boolean;
        mayContain(value: string): boolean;
    };
};
export = _default;
