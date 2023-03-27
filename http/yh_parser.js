
/////////////////////////////////


function __yh_cb_getplay_url(){
    //
    const _url = window.location.href;
    const _rand = Math.random();
    const _getplay_url = (_url.replace(/.*\/vp\/(\d+?)-(\d+?)-(\d+?)\.html.*/, '/_getplay?aid=$1&playindex=$2&epindex=$3') + '&r=' + _rand);
    return _getplay_url;
  }
  
  
  function __yh_play_ep_scroll(){
    const _href_url = window.location.href;
    const _refresl = _href_url.match(/\/vp\/(\d+?)-(\d+?)-(\d+?)\.html/);
    const _iPlay = Number(_refresl[2]);
    const _iEP = Number(_refresl[3]);
  
    //
    const _t_sel_movurl = '.tabs .main0 .movurl:nth-child(' + (_iPlay + 1) + ')';
    const _sel_lis = $(_t_sel_movurl + ' ul li');
    const _ep0_pos = _sel_lis[0].offsetTop;
    const _ep_pos = _sel_lis[_iEP].offsetTop;
    $(_t_sel_movurl + ' ul').scrollTop(_ep_pos - _ep0_pos);
    
    //
    const _t_sel_ep = _t_sel_movurl + ' ul li:nth-child(' + (_iEP + 1) + ')';
    const _sel_a = $(_t_sel_ep + ' a');
    _sel_a.css('background', '#FC8BBB');
    _sel_a.css('border', '1px solid #FC8BBB');
    _sel_a.css('color', '#FFF');
  
  }
  
  
  //////////////////////////////////////////////
  
  const __yh_g_exXP = [''];
  var __yh_g_isfullscn = false;
  var __yh_g_new_playleft_id = null;
  var __yh_margin_bak = '';
  
  
  function __yh_playfull_set(_in_id, _in_title_on, _in_exXP){
    if (!navigator.userAgent.match(/(iPhone|iPod|Android|mobile|blackberry|webos|incognito|webmate|bada|nokia|lg|ucweb|skyfire)/i)) {
      $('#'+_in_id).append('<a class="fullscn' + _in_exXP + '">' + _in_title_on + '</a>');
      
      //
      if(!__yh_g_isfullscn || !_in_exXP){
        $((".fullscn" + _in_exXP)).show();
      }
  
      //
      $('#'+_in_id).mouseover(function() {
        if(!__yh_g_isfullscn || !_in_exXP){
          $((".fullscn" + _in_exXP)).show();
        }
      }).mouseout(function() {
        $((".fullscn" + _in_exXP)).hide()
      });
  
      //
      $((".fullscn" + _in_exXP)).click(function() {
        if(!__yh_g_isfullscn){
          $((".fullscn" + '')).html('还原窗口');
          //
          const _new_ID = ("fullplayleft" + _in_exXP);
          $('#'+_in_id + ' iframe').css('width', '100%');
          $('#'+_in_id + ' iframe').css('height', '100%');
          __yh_margin_bak = $('#'+_in_id + ' iframe').css('margin');
          $('#'+_in_id + ' iframe').css('margin', '0px');
          $('#'+_in_id).attr("id", _new_ID);
  
          //
          __yh_g_new_playleft_id = _new_ID;
  
          //
          __yh_g_isfullscn = true;
        }
        else {
          $((".fullscn" + '')).html(_in_title_on);
          $('#'+_in_id + ' iframe').css('margin', __yh_margin_bak);
          $(('#' + __yh_g_new_playleft_id)).attr("id", _in_id);
  
          //
          __yh_g_isfullscn = false;
        }
      });
    };
  }
  
  function __yh_exp_playfull_set(_in_id){
    for (var i = 0; i < __yh_g_exXP.length; i++){
      const p1 = (__yh_g_exXP[i] ? ('网页' + __yh_g_exXP[i] + 'P') : '网页全屏');
      const p2 = (__yh_g_exXP[i] ? ('-' + __yh_g_exXP[i] + 'p') : '');
      __yh_playfull_set(_in_id, p1, p2);
    }
  }
  
  
  