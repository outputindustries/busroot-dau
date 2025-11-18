function typeError(tag, expected) {
    throw new TypeError(`unexpected tag 0x${tag.toString(16)} (${expected} expected)`);
}

// positive fixint: 0xxx xxxx
function posFixintTag(i) {
    return i & 0x7f;
}
function isPosFixintTag(tag) {
    return (tag & 0x80) === 0;
}
function readPosFixint(tag) {
    return tag & 0x7f;
}
// negative fixint: 111x xxxx
function negFixintTag(i) {
    return 0xe0 | (i & 0x1f);
}
function isNegFixintTag(tag) {
    return (tag & 0xe0) == 0xe0;
}
function readNegFixint(tag) {
    return tag - 0x100;
}
// fixstr: 101x xxxx
function fixstrTag(length) {
    return 0xa0 | (length & 0x1f);
}
function isFixstrTag(tag) {
    return (tag & 0xe0) == 0xa0;
}
function readFixstr(tag) {
    return tag & 0x1f;
}
// fixarray: 1001 xxxx
function fixarrayTag(length) {
    return 0x90 | (length & 0x0f);
}
function isFixarrayTag(tag) {
    return (tag & 0xf0) == 0x90;
}
function readFixarray(tag) {
    return tag & 0x0f;
}
// fixmap: 1000 xxxx
function fixmapTag(length) {
    return 0x80 | (length & 0x0f);
}
function isFixmapTag(tag) {
    return (tag & 0xf0) == 0x80;
}
function readFixmap(tag) {
    return tag & 0x0f;
}

function createWriteBuffer() {
    let view = new DataView(new ArrayBuffer(64));
    let n = 0;
    function need(x) {
        if (n + x > view.byteLength) {
            const arr = new Uint8Array(Math.max(n + x, view.byteLength + 64));
            arr.set(new Uint8Array(view.buffer.slice(0, n)));
            view = new DataView(arr.buffer);
        }
    }
    return {
        put(v) {
            need(v.byteLength);
            (new Uint8Array(view.buffer)).set(new Uint8Array(v), n);
            n += v.byteLength;
        },
        putI8(v) {
            need(1);
            view.setInt8(n, v);
            ++n;
        },
        putI16(v) {
            need(2);
            view.setInt16(n, v);
            n += 2;
        },
        putI32(v) {
            need(4);
            view.setInt32(n, v);
            n += 4;
        },
        putI64(v) {
            need(8);
            const neg = v < 0;
            if (neg) {
                v = -v;
            }
            let hi = (v / 0x100000000) | 0;
            let lo = (v % 0x100000000) | 0;
            if (neg) {
                // 2s complement
                lo = (~lo + 1) | 0;
                hi = lo === 0 ? (~hi + 1) | 0 : ~hi;
            }
            view.setUint32(n, hi);
            view.setUint32(n + 4, lo);
            n += 8;
        },
        putUi8(v) {
            need(1);
            view.setUint8(n, v);
            ++n;
        },
        putUi16(v) {
            need(2);
            view.setUint16(n, v);
            n += 2;
        },
        putUi32(v) {
            need(4);
            view.setUint32(n, v);
            n += 4;
        },
        putUi64(v) {
            need(8);
            view.setUint32(n, (v / 0x100000000) | 0);
            view.setUint32(n + 4, v % 0x100000000);
            n += 8;
        },
        putF(v) {
            need(8);
            view.setFloat64(n, v);
            n += 8;
        },
        ui8array() {
            return new Uint8Array(view.buffer.slice(0, n));
        },
    };
}
function createReadBuffer(buf) {
    let view = ArrayBuffer.isView(buf) ? new DataView(buf.buffer, buf.byteOffset, buf.byteLength) : new DataView(buf);
    let n = 0;
    return {
        peek() {
            return view.getUint8(n);
        },
        get(len) {
            n += len;
            const off = view.byteOffset;
            return view.buffer.slice(off + n - len, off + n);
        },
        getI8() {
            return view.getInt8(n++);
        },
        getI16() {
            n += 2;
            return view.getInt16(n - 2);
        },
        getI32() {
            n += 4;
            return view.getInt32(n - 4);
        },
        getI64() {
            n += 8;
            const hi = view.getInt32(n - 8);
            const lo = view.getUint32(n - 4);
            return hi * 0x100000000 + lo;
        },
        getUi8() {
            return view.getUint8(n++);
        },
        getUi16() {
            n += 2;
            return view.getUint16(n - 2);
        },
        getUi32() {
            n += 4;
            return view.getUint32(n - 4);
        },
        getUi64() {
            n += 8;
            const hi = view.getUint32(n - 8);
            const lo = view.getUint32(n - 4);
            return hi * 0x100000000 + lo;
        },
        getF32() {
            n += 4;
            return view.getFloat32(n - 4);
        },
        getF64() {
            n += 8;
            return view.getFloat64(n - 8);
        },
    };
}
function putBlob(buf, blob, baseTag) {
    const n = blob.byteLength;
    if (n <= 255) {
        buf.putUi8(baseTag);
        buf.putUi8(n);
    }
    else if (n <= 65535) {
        buf.putUi8(baseTag + 1);
        buf.putUi16(n);
    }
    else if (n <= 4294967295) {
        buf.putUi8(baseTag + 2);
        buf.putUi32(n);
    }
    else {
        throw new RangeError("length limit exceeded");
    }
    buf.put(blob);
}
function getBlob(buf) {
    const tag = buf.getUi8();
    let n;
    switch (tag) {
        case 192 /* Nil */:
            n = 0;
            break;
        case 196 /* Bin8 */:
        case 217 /* Str8 */:
            n = buf.getUi8();
            break;
        case 197 /* Bin16 */:
        case 218 /* Str16 */:
            n = buf.getUi16();
            break;
        case 198 /* Bin32 */:
        case 219 /* Str32 */:
            n = buf.getUi32();
            break;
        default:
            if (!isFixstrTag(tag)) {
                typeError(tag, "bytes or string");
            }
            n = readFixstr(tag);
    }
    return buf.get(n);
}
function putArrHeader(buf, n) {
    if (n < 16) {
        buf.putUi8(fixarrayTag(n));
    }
    else {
        putCollectionHeader(buf, 220 /* Array16 */, n);
    }
}
function getArrHeader(buf, expect?: any) {
    const tag = buf.getUi8();
    const n = isFixarrayTag(tag)
        ? readFixarray(tag)
        : getCollectionHeader(buf, tag, 220 /* Array16 */, "array");
    if (expect != null && n !== expect) {
        throw new Error(`invalid array header size ${n}`);
    }
    return n;
}
function putMapHeader(buf, n) {
    if (n < 16) {
        buf.putUi8(fixmapTag(n));
    }
    else {
        putCollectionHeader(buf, 222 /* Map16 */, n);
    }
}
function getMapHeader(buf, expect?: any) {
    const tag = buf.getUi8();
    const n = isFixmapTag(tag)
        ? readFixmap(tag)
        : getCollectionHeader(buf, tag, 222 /* Map16 */, "map");
    if (expect != null && n !== expect) {
        throw new Error(`invalid map header size ${n}`);
    }
    return n;
}
function putCollectionHeader(buf, baseTag, n) {
    if (n <= 65535) {
        buf.putUi8(baseTag);
        buf.putUi16(n);
    }
    else if (n <= 4294967295) {
        buf.putUi8(baseTag + 1);
        buf.putUi32(n);
    }
    else {
        throw new RangeError("length limit exceeded");
    }
}
function getCollectionHeader(buf, tag, baseTag, typename) {
    switch (tag) {
        case 192 /* Nil */:
            return 0;
        case baseTag: // 16 bit
            return buf.getUi16();
        case baseTag + 1: // 32 bit
            return buf.getUi32();
        default:
            typeError(tag, typename);
    }
}

const Any = {
    enc(buf, v) {
        typeOf(v).enc(buf, v);
    },
    dec(buf) {
        return tagType(buf.peek()).dec(buf);
    },
};
const Nil = {
    enc(buf) {
        buf.putUi8(192 /* Nil */);
    },
    dec(buf) {
        const tag = buf.getUi8();
        if (tag !== 192 /* Nil */) {
            typeError(tag, "nil");
        }
        return null;
    },
};
const Bool = {
    enc(buf, v) {
        buf.putUi8(v ? 195 /* True */ : 194 /* False */);
    },
    dec(buf) {
        const tag = buf.getUi8();
        switch (tag) {
            case 192 /* Nil */:
            case 194 /* False */:
                return false;
            case 195 /* True */:
                return true;
            default:
                typeError(tag, "bool");
        }
    },
};
const Int = {
    enc(buf, v) {
        if (-128 <= v && v <= 127) {
            if (v >= 0) {
                buf.putUi8(posFixintTag(v));
            }
            else if (v > -32) {
                buf.putUi8(negFixintTag(v));
            }
            else {
                buf.putUi8(208 /* Int8 */);
                buf.putUi8(v);
            }
        }
        else if (-32768 <= v && v <= 32767) {
            buf.putI8(209 /* Int16 */);
            buf.putI16(v);
        }
        else if (-2147483648 <= v && v <= 2147483647) {
            buf.putI8(210 /* Int32 */);
            buf.putI32(v);
        }
        else {
            buf.putI8(211 /* Int64 */);
            buf.putI64(v);
        }
    },
    dec(buf) {
        const tag = buf.getUi8();
        if (isPosFixintTag(tag)) {
            return readPosFixint(tag);
        }
        else if (isNegFixintTag(tag)) {
            return readNegFixint(tag);
        }
        switch (tag) {
            case 192 /* Nil */:
                return 0;
            // signed int types
            case 208 /* Int8 */:
                return buf.getI8();
            case 209 /* Int16 */:
                return buf.getI16();
            case 210 /* Int32 */:
                return buf.getI32();
            case 211 /* Int64 */:
                return buf.getI64();
            // unsigned int types
            case 204 /* Uint8 */:
                return buf.getUi8();
            case 205 /* Uint16 */:
                return buf.getUi16();
            case 206 /* Uint32 */:
                return buf.getUi32();
            case 207 /* Uint64 */:
                return buf.getUi64();
            default:
                typeError(tag, "int");
        }
    },
};
const Uint = {
    enc(buf, v) {
        if (v < 0) {
            throw new Error(`not an uint: ${v}`);
        }
        else if (v <= 127) {
            buf.putUi8(posFixintTag(v));
        }
        else if (v <= 255) {
            buf.putUi8(204 /* Uint8 */);
            buf.putUi8(v);
        }
        else if (v <= 65535) {
            buf.putUi8(205 /* Uint16 */);
            buf.putUi16(v);
        }
        else if (v <= 4294967295) {
            buf.putUi8(206 /* Uint32 */);
            buf.putUi32(v);
        }
        else {
            buf.putUi8(207 /* Uint64 */);
            buf.putUi64(v);
        }
    },
    dec(buf) {
        const v = Int.dec(buf);
        if (v < 0) {
            throw new RangeError("uint underflow");
        }
        return v;
    },
};
const Float = {
    enc(buf, v) {
        buf.putUi8(203 /* Float64 */);
        buf.putF(v);
    },
    dec(buf) {
        const tag = buf.getUi8();
        switch (tag) {
            case 192 /* Nil */:
                return 0;
            case 202 /* Float32 */:
                return buf.getF32();
            case 203 /* Float64 */:
                return buf.getF64();
            default:
                typeError(tag, "float");
        }
    },
};
const Bytes = {
    enc(buf, v) {
        putBlob(buf, v, 196 /* Bin8 */);
    },
    dec: getBlob,
};
const Str = {
    enc(buf, v) {
        const utf8 = (new TextEncoder()).encode(v).buffer;
        if (utf8.byteLength < 32) {
            buf.putUi8(fixstrTag(utf8.byteLength));
            buf.put(utf8);
        }
        else {
            putBlob(buf, utf8, 217 /* Str8 */);
        }
    },
    dec(buf) {
        return (new TextDecoder()).decode(getBlob(buf));
    },
};
const Time = {
    enc(buf, v) {
        const ms = v.getTime();
        buf.putUi8(199 /* Ext8 */);
        buf.putUi8(12);
        buf.putI8(-1);
        buf.putUi32((ms % 1000) * 1000000);
        buf.putI64(ms / 1000);
    },
    dec(buf) {
        const tag = buf.getUi8();
        switch (tag) {
            case 214 /* FixExt4 */: // 32-bit seconds
                if (buf.getI8() === -1) {
                    return new Date(buf.getUi32() * 1000);
                }
                break;
            case 215 /* FixExt8 */: // 34-bit seconds + 30-bit nanoseconds
                if (buf.getI8() === -1) {
                    const lo = buf.getUi32();
                    const hi = buf.getUi32();
                    // seconds: hi + (lo&0x3)*0x100000000
                    // nanoseconds: lo>>2 == lo/4
                    return new Date((hi + (lo & 0x3) * 0x100000000) * 1000 + lo / 4000000);
                }
                break;
            case 199 /* Ext8 */: // 64-bit seconds + 32-bit nanoseconds
                if (buf.getUi8() === 12 && buf.getI8() === -1) {
                    const ns = buf.getUi32();
                    const s = buf.getI64();
                    return new Date(s * 1000 + ns / 1000000);
                }
                break;
        }
        typeError(tag, "time");
    },
};
const Arr = TypedArr(Any);
const Map = TypedMap(Any, Any);
function TypedArr(valueT) {
    return {
        encHeader: putArrHeader,
        decHeader: getArrHeader,
        enc(buf, v) {
            putArrHeader(buf, v.length);
            v.forEach(x => valueT.enc(buf, x));
        },
        dec(buf) {
            const res: any[] = [];
            for (let n = getArrHeader(buf); n > 0; --n) {
                res.push(valueT.dec(buf));
            }
            return res;
        },
    };
}
function TypedMap(keyT, valueT) {
    return {
        encHeader: putMapHeader,
        decHeader: getMapHeader,
        enc(buf, v) {
            const props = Object.keys(v);
            putMapHeader(buf, props.length);
            props.forEach(p => {
                keyT.enc(buf, p);
                valueT.enc(buf, v[p]);
            });
        },
        dec(buf) {
            const res = {};
            for (let n = getMapHeader(buf); n > 0; --n) {
                const k = keyT.dec(buf);
                res[k] = valueT.dec(buf);
            }
            return res;
        },
    };
}
function structEncoder(fields) {
    const ordinals = Object.keys(fields);
    return (buf, v) => {
        putMapHeader(buf, ordinals.length);
        ordinals.forEach(ord => {
            const f = fields[ord];
            Int.enc(buf, Number(ord));
            f[1].enc(buf, v[f[0]]);
        });
    };
}
function structDecoder(fields) {
    return (buf) => {
        const res = {};
        for (let n = getMapHeader(buf); n > 0; --n) {
            const f = fields[Int.dec(buf)];
            if (f) {
                res[f[0]] = f[1].dec(buf);
            }
            else {
                Any.dec(buf);
            }
        }
        return res;
    };
}
function Struct(fields) {
    return {
        enc: structEncoder(fields),
        dec: structDecoder(fields),
    };
}
function unionEncoder(branches) {
    return (buf, v) => {
        putArrHeader(buf, 2);
        const ord = branches.ordinalOf(v);
        Int.enc(buf, ord);
        branches[ord].enc(buf, v);
    };
}
function unionDecoder(branches) {
    return (buf) => {
        getArrHeader(buf, 2);
        const t = branches[Int.dec(buf)];
        if (!t) {
            throw new TypeError("invalid union type");
        }
        return t.dec(buf);
    };
}
function Union(branches) {
    return {
        enc: unionEncoder(branches),
        dec: unionDecoder(branches),
    };
}
function typeOf(v) {
    switch (typeof v) {
        case "undefined":
            return Nil;
        case "boolean":
            return Bool;
        case "number":
            return !isFinite(v) || Math.floor(v) !== v ? Float
                : v < 0 ? Int
                    : Uint;
        case "string":
            return Str;
        case "object":
            return v === null ? Nil
                : Array.isArray(v) ? Arr
                    : v instanceof Uint8Array || v instanceof ArrayBuffer ? Bytes
                        : v instanceof Date ? Time
                            : Map;
        default:
            throw new TypeError(`unsupported type ${typeof v}`);
    }
}
function tagType(tag) {
    switch (tag) {
        case 192 /* Nil */:
            return Nil;
        case 194 /* False */:
        case 195 /* True */:
            return Bool;
        case 208 /* Int8 */:
        case 209 /* Int16 */:
        case 210 /* Int32 */:
        case 211 /* Int64 */:
            return Int;
        case 204 /* Uint8 */:
        case 205 /* Uint16 */:
        case 206 /* Uint32 */:
        case 207 /* Uint64 */:
            return Uint;
        case 202 /* Float32 */:
        case 203 /* Float64 */:
            return Float;
        case 196 /* Bin8 */:
        case 197 /* Bin16 */:
        case 198 /* Bin32 */:
            return Bytes;
        case 217 /* Str8 */:
        case 218 /* Str16 */:
        case 219 /* Str32 */:
            return Str;
        case 220 /* Array16 */:
        case 221 /* Array32 */:
            return Arr;
        case 222 /* Map16 */:
        case 223 /* Map32 */:
            return Map;
        case 214 /* FixExt4 */:
        case 215 /* FixExt8 */:
        case 199 /* Ext8 */:
            return Time;
        default:
            if (isPosFixintTag(tag) || isNegFixintTag(tag)) {
                return Int;
            }
            if (isFixstrTag(tag)) {
                return Str;
            }
            if (isFixarrayTag(tag)) {
                return Arr;
            }
            if (isFixmapTag(tag)) {
                return Map;
            }
            throw new TypeError(`unsupported tag ${tag}`);
    }
}

function encode(v, typ?: any) {
    const buf = createWriteBuffer();
    (typ || Any).enc(buf, v);
    return buf.ui8array();
}
function decode(buf, typ?: any) {
    return (typ || Any).dec(createReadBuffer(buf));
}

export { Any, Arr, Bool, Bytes, Float, Int, Map, Nil, Str, Struct, Time, TypedArr, TypedMap, Uint, Union, decode, encode, structDecoder, structEncoder, unionDecoder, unionEncoder };
//# sourceMappingURL=messagepack.es.js.map
