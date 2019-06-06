module StringMap = Map.Make(String);

module Option = {
  type t('a) = option('a);
  let return = x => Some(x);

  module Infix = {
    let (>>=) = (x, f) =>
      switch (x) {
      | Some(x) => f(x)
      | None => None
      };

    let (<!>) = (f, x) =>
      switch (x) {
      | Some(x) => Some(f(x))
      | None => None
      };

    let (<*>) = (f, x) =>
      switch (f, x) {
      | (Some(f), Some(x)) => Some(f(x))
      | _ => None
      };
  };

  let traverse = (f, lst) => {
    open Infix;
    let folder = (x, xs) => ((x, xs) => [x, ...xs]) <!> f(x) <*> xs;
    List.fold_right(folder, lst, return([]));
  };
};

module DecodeResult = Option;
type t('b, 'a) = 'b => DecodeResult.t('a);

let run = (json, decoder) => decoder(json);
let string = Js.Json.decodeString;
let nullableString = x => Some(Js.Json.decodeString(x));

let field = (field, decoder, x) =>
  DecodeResult.Infix.(
    Js.Json.decodeObject(x) >>= Js.Dict.get(_, field) >>= decoder
  );

let decodeString = Js.Json.decodeString;

let decodeMap = (decoder, doc) =>
  Array.fold_left(
    (xs, (key, value)) =>
      DecodeResult.Infix.(StringMap.add(key) <!> decoder(value) <*> xs),
    Some(StringMap.empty),
    Js.Dict.entries(doc),
  );

let decodeObject = (decoder, x) =>
  DecodeResult.Infix.(Js.Json.decodeObject(x) >>= decoder);

let decodeField = (field, decoder, x) =>
  DecodeResult.Infix.(Js.Dict.get(x, field) >>= decoder);

let pure = (x, _) => Some(x);
let apply = (f, x, y) =>
  switch (f(y), x(y)) {
  | (Some(f), Some(x)) => Some(f(x))
  | _ => None
  };

let return = (x, _) => Some(x);

let map = (f: 'a => 'b, x: t('x, 'a)): t('x, 'b) =>
  y =>
    switch (x(y)) {
    | None => None
    | Some(x) => Some(f(x))
    };

let decodeArray = (decoder: t('x, 'a)): t('x, list('a)) =>
  DecodeResult.Infix.(
    x =>
      Array.to_list
      <!> Js.Json.decodeArray(x)
      >>= DecodeResult.traverse(decoder)
  );

let (<*>) = apply;
let (<$>) = map;
let (<!>) = (x, f) => map(f, x);
let (>>=) = (x, f, js) =>
  switch (x(js)) {
  | Some(x) => f(x)
  | None => None
  };
