var req = new XMLHttpRequest();
var ids; // array of emperor ID's
var idx = -1;   // index of the current emperor in the ids array

function $(id)  {  return document.getElementById(id); }

function load_ids() { return eval('(' + request("/demo/emperors") + ')'); }
function next() {  if(idx+1 < ids.length) { idx++ ; get_emperor() } }
function prev() {  if(idx > 0)            { idx-- ; get_emperor() } }

function request(url) {
  req.open("GET",url,false);
  req.send(null);
  return req.responseText;
};

function get_emperor() {
  var emp_id = ids[idx].id;
  var i; 
  $("photo").src = "/demo/photo/" + emp_id;
  $("content").innerHTML = request("/demo/emperor/" + emp_id);
  var com_list = eval('(' + request("/demo/comment?emp_id=" + emp_id) +')');
  $("comments").innerHTML = "";
  for(i=0; i < com_list.length; i++) $("comments").innerHTML += 
    "&bull; " + com_list[i].comment + "<br />\n";
};
