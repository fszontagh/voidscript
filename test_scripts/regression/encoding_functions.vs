// Roadmap Tier 1: encoding helpers - url/hex/html encode+decode and ord/chr.
printnl(url_encode("a b&c=d/e"));                    // a%20b%26c%3Dd%2Fe
printnl(url_decode("a%20b%26c%3Dd+f"));              // a b&c=d f
printnl(hex_encode("AB"));                           // 4142
printnl(hex_decode("4142"));                         // AB
printnl(html_escape("<a>'&'</a>"));                  // &lt;a&gt;&#39;&amp;&#39;&lt;/a&gt;
printnl(html_unescape("&lt;a&gt;&amp;&quot;&#39;")); // <a>&"'
printnl(ord("A"), " ", chr(66));                     // 65 B
printnl("done");
