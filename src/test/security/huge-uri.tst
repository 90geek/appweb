/*
    Very large URI test (3MB)
 */ 
const HTTP: Uri = App.config.uris.http || "127.0.0.1:4100"

//  This writes a 2MB URI

//  Data is 100K after this
let data = ""
for (i in 1000) {
    data += "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678\n"
}

/*
    Test LimitUri
 */
let s = new Socket
s.connect(HTTP.address)
let count = 0
try {
    count += s.write("GET ")
    count += s.write(data)
    count += s.write(" HTTP/1.1\r\n\r\n")
} catch {
    App.log.error("Write failed. Wrote  " + count + " of " + data.length + " bytes.")
    //  Check appweb.conf LimitRequestHeader. This must be sufficient to accept the write the header.
}

response = new ByteArray
while ((n = s.read(response, -1)) > 0) { }
if (!response.toString().contains("HTTP/1.1 414 Request-URI Too Large")) {
    print("RESPONSE IS " + response)
}
assert(response.toString().contains("HTTP/1.1 414 Request-URI Too Large"))
s.close()

//  Check server still up
http = new Http
http.get(HTTP + "/index.html")
assert(http.status == 200)
http.close()
