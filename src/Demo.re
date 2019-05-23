open Index;
open Index.Handler;

[@bs.val] external setTimeout: (unit => unit, int) => float = "setTimeout";

let delay = (x, req) =>
  Async(
    cb => Handler.(setTimeout(() => cb(Continue(x, req)), 1000) |> ignore),
  );

let middleware =
  choose([
    path("path") >=> sendText("Hello from /path"),
    path("delay") >=> delay >=> sendText("This renders after 1 second"),
    sendText("Not found"),
  ]);

let server = createServerM(middleware);

%raw
{|
const http = require('http');
let httpServer = http.createServer(server);
httpServer.listen(4000);
|};
