module Url = {
  type t;
  [@bs.module "url"] external parse: string => t = "";
  [@bs.get] external path: t => string = "";
};

module Request = {
  type t;

  [@bs.get] external url: t => string = "";
};

module Response = {
  type t;
  [@bs.send.pipe: t] external end_: string => unit = "end";
};

type requestListener = (Request.t, Response.t) => unit;
