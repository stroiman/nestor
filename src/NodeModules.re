/* Standard node.js http module */
module Http = {
  module Request = {
    module Body = {
      type t;

      [@bs.send] external on: (t, string, string => unit) => unit = "";
    };
    type t;

    [@bs.get] external body: t => Body.t = "%identity";
    [@bs.get] external headers: t => Js.Dict.t(string) = "";
    [@bs.get] external headers: t => Js.Dict.t(string) = "";
    [@bs.get] external method: t => string = "method";
    [@bs.get] external url: t => string = "";
  };

  module Response = {
    type t;

    [@bs.send.pipe: t] external writeHead: int => unit = "";
    [@bs.send.pipe: t] external end_: string => unit = "end";
  };

  type requestListener = (. Request.t, Response.t) => unit;
};

/* Standard node.js url module */
module Url = {
  type t;
  [@bs.module "url"] external parse: string => t = "";
  [@bs.get] external path: t => string = "";
};

/* npm package 'cookie' */
module Cookie = {
  [@bs.module "cookie"] external parse: string => Js.Dict.t(string) = "";
};
