open Index;
open Index.Handler;

let middleware = path("path") >=> sendText("Hello from /path");
let server = createServerM(middleware);

%raw
{|
const http = require('http');
let httpServer = http.createServer(server);
httpServer.listen(4000);
|};
