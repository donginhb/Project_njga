<?php
namespace Home\Controller;
use Think\Controller;
class PlatformController extends Controller {
    public function index(){

        $model = M()->table("VideoManagePlatformInfo");

        $info = $model->find();
        //dump($info);

        if ($info){
            $info["lasttasktime"] = date("Y-m-d H:i:s",$info["lasttasktime"]);

            $status = $info["compresstaskstatus"];
            //dump($status);
            if ($status == "0")
                $info["compresstaskstatus"] = "初始状态";
            if ($status == "1")
                $info["compresstaskstatus"] = "准备获取";
            if ($status == "2")
                $info["compresstaskstatus"] = "正在获取处理";
            if ($status == "3")
                $info["compresstaskstatus"] = "获取失败";
            if ($status == "4")
                $info["compresstaskstatus"] = "获取成功,准备分配";
            if ($status == "5")
                $info["compresstaskstatus"] = "正在分配处理";
            if ($status == "6")
                $info["compresstaskstatus"] = "没有分配成功";
            if ($status === "7")
                $info["compresstaskstatus"] = "分配成功";

            $errorcode = intval($info["errorcode"]);
            switch ($errorcode){
                case 0x02001;
                    $info["errorreason"] = "没有可用的ZRV设备";
                    break;
                case 0x02002;
                    $info["errorreason"] = "调用 Webservice 查询接口失败";
                    break;
                case 0x02003;
                    $info["errorreason"] = "调用 Webservice 更新接口失败";
                    break;
                case 0x02004;
                    $info["errorreason"] = "调用 Webservice 插入接口失败";
                    break;
                case 0x02005;
                    $info["errorreason"] = "调用 Webservice 删除接口失败";
                    break;
                case 0x02006;
                    $info["errorreason"] = "解析 Webservice 删除接口失败";
                    break;
                case 0x02007;
                    $info["errorreason"] = "没有获取到压缩任务";
                    break;

            }
        }

        $this->assign("info",$info);

        $this->display();
    }

    public function update(){

        $ip = I('ip', '', 'htmlspecialchars');

        $info = M()->table("VideoManagePlatformInfo")->where(array('platformip'=>$ip))->find();

        $data["status"] = 0;
        if ($info){
            if($ip != $info['platformip']){
                $updatedata = array('PlatformIP'=>$ip);
                $ret = M()->table("VideoManagePlatformInfo")-> where(array('platformip'=>$ip))->setField($updatedata);

                if (!$ret){
                    $data["status"] = -1;
                    $data["errormsg"] = "更新数据失败";
                }
            }
        }else{
            // 插入数据
            $insertData = array('PlatformIP'=>$ip);
            M()->table("VideoManagePlatformInfo")->add($insertData);
        }

        $this->ajaxReturn($data);

    }

}