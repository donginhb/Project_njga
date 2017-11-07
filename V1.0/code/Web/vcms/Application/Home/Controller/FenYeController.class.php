<?php
namespace Home\Controller;
use Think\Controller;
class FenYeController extends Controller {
    //分页属性
    public $pageSize;
    public $rowCount;
    public $pageNow;
    public $pageCount;
    public $navigate1;
    public $navigate2;
    public $funccall;
    public static $Instance = NULL;

    //初始分页属性
    public function __construct() {
        parent::__construct();
        $this->pageSize=15;
        $this->pageNow=1;
        $this->pageCount=1;
        $this->navigate="";
    }

    static public function Instance() {
        if (!isset(self::$Instance)) {

            self::$Instance = new FenYeController();
        }
        return self::$Instance;
    }
    /**
     * 分页导航样式1
     */
    public function getNavigate(){
        if($this->pageCount==0){
            $this->navigate="<div style='margin-top:5px;height:40px;line-height:40px;'>";
            $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(__ROOT__/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(__ROOT__/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate.="<span style='margin:0 5px;'>"."<input type='jump' id='test' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
            $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(__ROOT__/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(__ROOT__/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;'>GO</span> ";
            $this->navigate.="</div>";
        }else{
            if($this->pageCount==1){
                $this->navigate="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;'>GO</span> ";
                $this->navigate.="</div>";
            }
            else{
                if($this->pageNow == 1){
                    $nextpage = $this->pageNow + 1;
                    $this->navigate="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage($nextpage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'onclick='setPage($this->pageCount)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='setPage(document.getElementById(\"jump\").value)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate.="</div>";
                }
                if($this->pageNow != 1 && $this->pageNow != $this->pageCount){
                    $nextpage = $this->pageNow + 1;
                    $prepage=$this->pageNow - 1;
                    $this->navigate="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage(1)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage($prepage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage($nextpage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage($this->pageCount)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='setPage(document.getElementById(\"jump\").value)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate.="</div>";
                }
                if($this->pageNow == $this->pageCount){
                    $prepage=$this->pageNow - 1;
                    $this->navigate="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage(1)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;' onclick='setPage($prepage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='setPage(document.getElementById(\"jump\").value)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate.="</div>";
                }
            }
        }
        return $this->navigate;
    }
    /**
     * 分页导航样式1
     */
    public function getNavigate2(){
        if($this->pageCount==0){
            $this->navigate2="<div style='margin-top:5px;height:40px;line-height:40px;'>";
            $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate2.="<span style='margin:0 5px;'>"."<input type='jump' id='test' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
            $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
            $this->navigate2.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;'>GO</span> ";
            $this->navigate2.="</div>";
        }else{
            if($this->pageCount==1){
                $this->navigate2="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate2.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                $this->navigate2.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;'>GO</span> ";
                $this->navigate2.="</div>";
            }
            else{
                if($this->pageNow == 1){
                    $nextpage = $this->pageNow + 1;
                    $this->navigate2="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}($nextpage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'onclick='{$this->funccall}($this->pageCount)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='{$this->funccall}(document.getElementById(\"jump\").value)'  onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate2.="</div>";
                }
                if($this->pageNow != 1 && $this->pageNow != $this->pageCount){
                    $nextpage = $this->pageNow + 1;
                    $prepage=$this->pageNow - 1;
                    $this->navigate2="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}(1)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}($prepage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}($nextpage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}($this->pageCount)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='{$this->funccall}(document.getElementById(\"jump\").value)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate2.="</div>";
                }
                if($this->pageNow == $this->pageCount){
                    $prepage=$this->pageNow - 1;
                    $this->navigate2="<div style='margin-top:5px;height:40px;line-height:40px;'>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left4.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}(1)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;width:29px;background: url(".__ROOT__."/Public/vcms/images/page_aero_left3.gif) no-repeat center; border:1px solid #ccc;' onclick='{$this->funccall}($prepage)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;'>"."<input type='text' id='jump' style='width:35px;height:28px;text-align:center;border:1px solid #ccc;' value=".$this->pageNow." /> "."/{$this->pageCount}页</span>";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate2.="<span style='margin-left:5px;padding:5px 10px;background: url(".__ROOT__."/Public/vcms/images/page_aero_right2.gif) no-repeat center; border:1px solid #ccc;'></span> ";
                    $this->navigate2.="<span style='margin:0 5px;border:1px solid #ccc;padding:5px 5px;width:35px;height:28px;text-align:center;' onclick='{$this->funccall}(document.getElementById(\"jump\").value)' onmouseover='ChangeColor(this)' onmouseout='RestoreColor(this)'>GO</span> ";
                    $this->navigate2.="</div>";
                }
            }
        }
        return $this->navigate2;
    }
}