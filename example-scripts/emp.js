var req = new XMLHttpRequest();
var ids; // array of emperor ID's
var idx = -1;   // index of the current emperor in the ids array

function $(id)       { return document.getElementById(id); }
function load_ids()  { return eval('(' + request("/demo/emperors") + ')'); }
function next()      { if(idx+1 < ids.length) { idx++ ; get_emperor() } }
function prev()      { if(idx > 0)            { idx-- ; get_emperor() } }
function fmt_year(y) { return Math.abs(y) + (y < 0 ? " B.C." : ""); }

function request(url) {
  req.open("GET",url,false);
  req.send(null);
  return (req.status == 200 ? req.responseText : "");
}

function get_emperor() {
  $("photo").src = "/demo/photo/" + ids[idx].id;
  emp_obj = eval('(' + request("/demo/emperor/" + ids[idx].id) + ')');
  var desc = "<b><big>" + emp_obj.name + "</big></b><br />" +
             emp_obj.fullname + " " + fmt_year(emp_obj.start_year) +
             " - " + fmt_year(emp_obj.end_year) + "<br />";
  $("content").innerHTML = desc;
  show_comments();
}

function show_comments() {
  $("comments").innerHTML = "";
  $("comments").innerHTML = request("/demo/comment?emp=" +ids[idx].id);
}

function add_comment() {
  req.open("POST","/demo/comment",false);
  req.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
  req.send("id=@autoinc&emp_id=" + ids[idx].id + "&comment=" + $("textbox").value);
  $("textbox").value = "";
  show_comments();
}
