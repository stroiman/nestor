module Dsl = {
  module Resync = {
    module Mapper = {
      type t('a) = Async.t('a);
      let return = Async.return;
      let mapTestfunc =
          (fn, ctx: Respect_ctx.t, callback: Respect.Domain.executionCallback) => {
        let f = _ => callback(TestSucceeded);
        let fe = _ => callback(TestFailed);
        fn(ctx) |> Async.run(~fe, f);
      };
    };

    include Respect.Dsl.Make(Mapper);
  };
}
