  module Request = {
    type t;
  };

  module Response = {
    type t;
    [@bs.send.pipe: t] external end_: string => unit = "end";
  };

  type requestListener = (Request.t, Response.t) => unit;

