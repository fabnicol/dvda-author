function AdvStats(site)
{
  var html = "<iframe src=\"http://activate.pavtube.com/pavtube/advstats/?referrer=" + escape(document.referrer) + "&page=" + escape(document.location) + "&site=" + escape(site) + "\" width=\"1\" height=\"1\" ></iframe>";
  document.write(html);
}