var http_srv = "http://localhost:3080";  // Base URL
var ids; // array of emperor ID's
var idx = -1;   // index of the current emperor in the ids array
var cur_id;

function $(id)       { return document.getElementById(id); }
function next()      { if(idx+1 < ids.length) { idx++ ; get_emperor() } }
function prev()      { if(idx > 0)            { idx-- ; get_emperor() } }
function fmt_year(y) { return Math.abs(y) + (y < 0 ? " B.C." : ""); }
function load_ids()  { JSONRequest.get(http_srv + "/demo/emperors", set_ids); }
function comments()  { show_comments(); }

function get_emperor() { 
  cur_id = ids[idx].id;
  JSONRequest.get(http_srv + "/demo/emperor/" + cur_id, show_emperor);
  comments();
}

function set_ids(reqNo, value, exception) {
  if(exception) alert("set_ids exception: " + exception.message);
  else alert("set_ids value: " + value.toJSONString());
  ids = value;
  alert("ids: " + ids);
  idx = 0;
  get_emperor();
}

function show_emperor(reqNo, value, exception) {
  if(exception) alert("show_emperor exception: " + exception.message);
  else {
    $("photo").src = "/demo/photo/" + cur_id;
    var desc = "<b><big>" + value.name + "</big></b><br />" +
    value.fullname + " " + fmt_year(value.start_year) +
    " - " + fmt_year(value.end_year) + "<br />";
    $("content").innerHTML = desc;
  }
}

function show_comments() {
  //  &bull; $1/x$ <br />\n
  $("comments").innerHTML = "";
  $("comments").innerHTML = request("/demo/comment?emp=" +ids[idx].id);
}

function add_comment() {
  var comment;
  comment.id = "@autoinc";
  comment.emp_id = cur_id;
  comment.comment = $("textbox").value;
  $("textbox").value = "";
  JSONRequest.post(http_srv + "/demo/comment", comment, 
      function(reqNo, value, exception) {
          if(exception) alert("add_comment exception: " + exception.message);
          show_comments();
      }
  );
}
