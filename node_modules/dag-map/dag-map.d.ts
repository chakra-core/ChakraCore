export interface Callback<T> {
    (key: string, value: T | undefined): void;
}
export declare type MaybeStringOrArray = string | string[] | undefined;
/**
 * A topologically ordered map of key/value pairs with a simple API for adding constraints.
 *
 * Edges can forward reference keys that have not been added yet (the forward reference will
 * map the key to undefined).
 */
export default class DAG<T> {
    private _vertices;
    /**
     * Adds a key/value pair with dependencies on other key/value pairs.
     *
     * @public
     * @param key    The key of the vertex to be added.
     * @param value  The value of that vertex.
     * @param before A key or array of keys of the vertices that must
     *               be visited before this vertex.
     * @param after  An string or array of strings with the keys of the
     *               vertices that must be after this vertex is visited.
     */
    add(key: string, value: T | undefined, before?: MaybeStringOrArray, after?: MaybeStringOrArray): void;
    /**
     * @deprecated please use add.
     */
    addEdges(key: string, value: T | undefined, before?: MaybeStringOrArray, after?: MaybeStringOrArray): void;
    /**
     * Visits key/value pairs in topological order.
     *
     * @public
     * @param callback The function to be invoked with each key/value.
     */
    each(callback: Callback<T>): void;
    /**
     * @deprecated please use each.
     */
    topsort(callback: Callback<T>): void;
}
