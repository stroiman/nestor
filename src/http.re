module Url = {
  type t;
  [@bs.module "url"] external parse: string => t = "";
  [@bs.get] external path: t => string = "";
};

module Request = {
  type t;

  [@bs.get] external url: t => string = "";
  [@bs.get] external headers: t => Js.Dict.t(string) = "";
};

module Response = {
  type t;
  [@bs.send.pipe: t] external end_: string => unit = "end";
};

type requestListener = [@bs] (Request.t, Response.t) => unit;

module Cookie = {
  [@bs.module "cookie"] external parse: string => Js.Dict.t(string) = "";
};
