<?php
namespace Home\Controller;
use Think\Controller;
class DeviceController extends Controller {
    public function index(){

        $model = M()->table("ZRVDeviceInfo");

        $list = $model->field("ZRVDeviceInfo.`ID`,ZRVDeviceInfo.`DeviceIP`,ZRVDeviceInfo.`Status`,ZRVDeviceInfo.`Expires`,COUNT(ZRVDeviceIP) AS num")->join('LEFT JOIN ZRVCompressTaskAssignInfo on ZRVDeviceInfo.DeviceIP=ZRVCompressTaskAssignInfo.ZRVDeviceIP  GROUP BY DeviceIP')->select();

        $this->assign("list",$list);

        $this->display();
    }

    public function delete($ids){
        $ids = I('ids', '', 'htmlspecialchars');

        $iddata = explode(',',$ids);

        $model = M()->table("ZRVDeviceInfo");

        for($index=0;$index<count($iddata);$index++)
        {
            $model->where('id='.$iddata[$index])->delete();
        }


        $data["status"] = 0;

        $this->ajaxReturn($data);
    }
    
    
    public function detail(){
        $deviceip = I('ZRVDeviceIP', '', 'htmlspecialchars');
        $pagenow = I('pagenow', 1, 'htmlspecialchars');
        $pagesize = I('pagesize', 10, 'htmlspecialchars');
        //var_dump($deviceip."|".$pagenow."|".$pagesize);exit;
        $count = M()->table("ZRVCompressTaskAssignInfo")->where(array('ZRVDeviceIP'=>$deviceip))->count();
        
        FenYeController::Instance()->rowCount=$count;
        FenYeController::Instance()->pageNow=$pagenow;
        FenYeController::Instance()->pageSize=$pagesize;
        $pagecount=ceil(FenYeController::Instance()->rowCount/FenYeController::Instance()->pageSize);
        FenYeController::Instance()->pageCount=$pagecount;
        $start=(FenYeController::Instance()->pageNow-1) * FenYeController::Instance()->pageSize;
        $end=FenYeController::Instance()->pageSize;
        $ZRVDeviceInfo = M()->table("ZRVCompressTaskAssignInfo")->field('recordnum,filename,uploadtime,taskcreatetime,zrvdeviceip,taskstatus')->where(array('ZRVDeviceIP'=>$deviceip))->order("ZRVDeviceIP,uploadtime")->limit($start,$end)->select();
        
        if ($ZRVDeviceInfo){
            
            for($i=0;$i<count($ZRVDeviceInfo);$i++){
                $ZRVDeviceStr.="<tr>";
                //$taskstatus = intval($ZRVDeviceInfo[$i]["taskstatus"]);
                //if ($taskstatus == 1)
                //    $ZRVDeviceInfo[$i]["taskstatus"]="正在压缩任务";
                //if ($taskstatus == 2)
                //    $ZRVDeviceInfo[$i]["taskstatus"]="压缩完成";
                //if ($taskstatus == 3)
                //    $ZRVDeviceInfo[$i]["taskstatus"]="压缩超时";
                //if(!empty($pagetype)){
                //    $ZRVDeviceInfo[$i]["taskstatus"]="<a href='javascript:void(0);' onclick=gettaskdetail2('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a>";
                //    FenYeController::Instance()->funccall="setPage2";
                //}else{
                //    $ZRVDeviceInfo[$i]["taskstatus"]="<a href='javascript:void(0);' onclick=gettaskdetail('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a>";
                //}
                $ZRVDeviceInfo[$i]["taskstatus"]="<a href='javascript:void(0);' onclick=gettaskdetail('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a>";


                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["recordnum"]."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["filename"]."</td>";
                $ZRVDeviceStr.="<td>".date('Y-m-d H:i:s',$ZRVDeviceInfo[$i]["uploadtime"])."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["taskcreatetime"]."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["zrvdeviceip"]."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["taskstatus"]."</td>";
                $ZRVDeviceStr.="</tr>";
            }
        }
        //FenYeController::Instance()->getNavigate();
        //$this->ajaxReturn($ZRVDeviceInfo);
        echo $ZRVDeviceStr."<tr><td colspan=6 style='text-align:right;'>".FenYeController::Instance()->getNavigate()."</td></tr>";
    }
    
    public function getHisCompressTask(){
        
        $getHisCompressTaskByHour=I("getHisCompressTaskByHour",false,"htmlspecialchars");
        $getHisCompressTaskByDay=I("getHisCompressTaskByDay",false,"htmlspecialchars");
        $deviceip=I("zrvdeviceip","","htmlspecialchars");
        if($getHisCompressTaskByHour){
            $hour=date("H");
            $stquerytime=date("Y-m-d",time());
            $edquerytime=date("Y-m-d H:i:s",time());
            $ZRVDeviceInfo = M()->table("ZRVCompressTaskInfo")->field('recordnum,filename,uploadtime,taskcreatetime,zrvdeviceip,taskstatus')->where("ZRVDeviceIP='$deviceip' AND taskcreatetime>='$stquerytime' AND taskcreatetime<='$edquerytime'")->order("ZRVDeviceIP,uploadtime")->select();
            $hourstr=array();
            $hourdata=array();
            for($i=1;$i<=($hour+1);$i++){
                $st=$i-1;
                $et=$i;
                array_push($hourstr,"$st-$et");
                $sumcount=0;
                for($j=0;$j<count($ZRVDeviceInfo);$j++){
                    if(date("H",strtotime($ZRVDeviceInfo[$j]["taskcreatetime"]))==$st){
                        $sumcount++;
                    }
                }
                array_push($hourdata,$sumcount);
            }
            $res=array($hourstr,$hourdata);
            $this->ajaxReturn($res);
        }else if($getHisCompressTaskByDay){
            $timebegin=date("Y-m-d",strtotime(I("timebegin","","htmlspecialchars")));
            $timeend=date("Y-m-d",(strtotime(I("timeend","","htmlspecialchars"))+86400));
            
            $ZRVDeviceInfo = M()->table("ZRVCompressTaskInfo")->field('recordnum,filename,uploadtime,taskcreatetime,zrvdeviceip,taskstatus')->where("ZRVDeviceIP='$deviceip' AND taskcreatetime>='$timebegin' AND taskcreatetime<'$timeend'")->order("ZRVDeviceIP,uploadtime")->select();
            //echo  M()->table("ZRVCompressTaskInfo")->getLastSql();
            $days=(strtotime($timeend)-strtotime($timebegin)-86400)/86400;
            $daystr=array();
            $daydata=array();
            for($i=0;$i<=$days;$i++){
                $dayname=date("Y-m-d",strtotime($timebegin)+86400*$i);
                //array_push($daystr,[date("Y-m-d",strtotime($timebegin)+86400*$i)]);
                array_push($daystr,$dayname);
                $sumcount=0;
                for($j=0;$j<count($ZRVDeviceInfo);$j++){
                    if(date("Y-m-d",strtotime($ZRVDeviceInfo[$j]["taskcreatetime"]))==$dayname){
                        $sumcount++;
                    }
                }
                array_push($daydata,$sumcount);
            }
            $res=array($daystr,$daydata);
            $this->ajaxReturn($res);
        }
    }
}