<?php
namespace Home\Controller;
use Org\Util\Date;
use Think\Controller;


class TaskassignController extends Controller {
    public function index(){
        
        /**
         * 直接把上个页面写的sql语句复制过来
         * @return $zvrlist 设备状态及对应设备当前压缩任务数
         * */
        $model = M()->table("ZRVDeviceInfo");

        $zvrlist = $model->field("ZRVDeviceInfo.`ID`,ZRVDeviceInfo.`DeviceIP`,ZRVDeviceInfo.`Status`,ZRVDeviceInfo.`Expires`,COUNT(ZRVDeviceIP) AS num")->join('LEFT JOIN ZRVCompressTaskAssignInfo on ZRVDeviceInfo.DeviceIP=ZRVCompressTaskAssignInfo.ZRVDeviceIP  GROUP BY DeviceIP')->select();

        
        $this->assign("zvrlist",$zvrlist);
        $this->assign("zvrlistcount",count($zvrlist));
        $zvrlisttaskcount=0;
        foreach($zvrlist as $value){
            $zvrlisttaskcount+=$value["num"];
        }
        $this->assign("zvrlisttaskcount",$zvrlisttaskcount);
        
        $res_fenye=M()->table("ZRVCompressTaskAssignInfo")->join("ZRVDeviceInfo ON ZRVCompressTaskAssignInfo.ZRVDeviceIP=ZRVDeviceInfo.DeviceIP")->order('uploadtime desc')->select();
        $res_nofenye=M()->table("ZRVCompressTaskAssignInfo")->join("ZRVDeviceInfo ON ZRVCompressTaskAssignInfo.ZRVDeviceIP=ZRVDeviceInfo.DeviceIP")->select();

        $ERR_0x01001_NUM=0;
        $ERR_0x01002_NUM=0;
        $ERR_0x01003_NUM=0;
        $ERR_0x01004_NUM=0;
        $ERR_0x01005_NUM=0;
        $ERR_0x01006_NUM=0;
        $ERR_0x01007_NUM=0;
        $ERR_0x01008_NUM=0;
        $ERR_0x01009_NUM=0;
        $ERR_0x02000_NUM=0;
        
        $failnum=0;    
        $successnum=0;
        
        for ($i=0;$i<count($res_fenye);$i++){
            $date = date("Y-m-d H:i:s",$res_fenye[$i]["uploadtime"]);
            $res_fenye[$i]["uploadtime"] = $date;

            $s = intval($res_fenye[$i]["taskstatus"]);
            switch ($s){
                case 0;
                    $res_fenye[$i]["taskstatus"]="未开始压缩任务";
                    break;
                case 1;
                    $res_fenye[$i]["taskstatus"]="正在压缩任务";
                    break;
                case 2;
                    $res_fenye[$i]["taskstatus"]="压缩完成";
                    break;
                case 3;
                    $res_fenye[$i]["taskstatus"]="压缩超时";
                    break;
            }

            $r = intval($res_fenye[$i]["taskresult"]);
            switch ($r){
                case 0;
                    $res_fenye[$i]["taskresult"]="等待结果";
                    break;
                case 1;
                    $res_fenye[$i]["taskresult"]="成功";
                    break;
                case 2;
                    $res_fenye[$i]["taskresult"]="失败";
                    break;
            }
        }
        for($j=0;$j<count($res_nofenye);$j++){
            $ec = intval($res_nofenye[$j]["errorcode"]);
            switch($ec){
                case 0x01001:
                    $failnum++;
                    $ERR_0x01001_NUM++;
                    break;
                case 0x01002:
                    $failnum++;
                    $ERR_0x01002_NUM++;
                    break;
                case 0x01003:
                    $failnum++;
                    $ERR_0x01003_NUM++;
                    break;
                case 0x01004:
                    $failnum++;
                    $ERR_0x01004_NUM++;
                    break;
                case 0x01005:
                    $failnum++;
                    $ERR_0x01005_NUM++;
                    break;
                case 0x01006:
                    $failnum++;
                    $ERR_0x01006_NUM++;
                    break;
                case 0x01007:
                    $failnum++;
                    $ERR_0x01007_NUM++;
                    break;
                case 0x01008:
                    $failnum++;
                    $ERR_0x01008_NUM++;
                    break;
                case 0x01009:
                    $failnum++;
                    $ERR_0x01009_NUM++;
                    break;
                case 0x02000:
                    $failnum++;
                    $ERR_0x02000_NUM++;
                    break;    
                case 200:
                    $successnum++;
                    break;
            }
        }
        
        $res_nofenye['failnum']=$failnum;
        $res_nofenye['successnum']=$successnum;
        $res_nofenye["ERR_0x01001_NUM"]=$ERR_0x01001_NUM;
        $res_nofenye["ERR_0x01002_NUM"]=$ERR_0x01002_NUM;
        $res_nofenye["ERR_0x01003_NUM"]=$ERR_0x01003_NUM;
        $res_nofenye["ERR_0x01004_NUM"]=$ERR_0x01004_NUM;
        $res_nofenye["ERR_0x01005_NUM"]=$ERR_0x01005_NUM;
        $res_nofenye["ERR_0x01006_NUM"]=$ERR_0x01006_NUM;
        $res_nofenye["ERR_0x01007_NUM"]=$ERR_0x01007_NUM;
        $res_nofenye["ERR_0x01008_NUM"]=$ERR_0x01008_NUM;
        $res_nofenye["ERR_0x01009_NUM"]=$ERR_0x01009_NUM;
        $res_nofenye["ERR_0x02000_NUM"]=$ERR_0x02000_NUM;

        $this->assign("errorinfo",$res_nofenye);
        //$this->assign("allres",$allres);

        //$this->display();

        $deviceip = I('deviceip', '', 'htmlspecialchars');
        if (strlen($deviceip)>0){
            $map['ZRVDeviceIP'] = $deviceip;
            $this->assign('deviceip',$deviceip);
        }

        $begintime = I('begintime', '', 'htmlspecialchars');
        $endtime = I('endtime', '', 'htmlspecialchars');
        if(strlen($begintime)>0 && $begintime != "NaN" && strlen($endtime)>0 && $endtime != "NaN"){
            $map['UploadTime'] =  array(array("gt",intval($begintime)),array("lt",intval($endtime)));
            $begintime_str = date("Y-m-d H:i:s",intval($begintime));
            $this->assign('begintime_str',$begintime_str);
            $endtime_str = date("Y-m-d H:i:s",intval($endtime));
            $this->assign('endtime_str',$endtime_str);
        }else{
            if (strlen($begintime)>0 && $begintime != "NaN"){
                $map['UploadTime'] =  array("gt",intval($begintime));

                $begintime_str = date("Y-m-d H:i:s",intval($begintime));
                $this->assign('begintime_str',$begintime_str);
            }
            if (strlen($endtime)>0 && $endtime != "NaN"){
                $map['UploadTime'] =  array("lt",intval($endtime));
                $endtime_str = date("Y-m-d H:i:s",intval($endtime));
                $this->assign('endtime_str',$endtime_str);
            }
        }

        // 任务创建时间处理

        $createbegintime = I('createbegintime', '', 'htmlspecialchars');
        $createendtime = I('createendtime', '', 'htmlspecialchars');
        if(strlen($createbegintime)>0 && strlen($createendtime)>0){
            $map['TaskCreateTime'] =  array(array("gt",$createbegintime),array("lt",$createendtime));
            $this->assign('createbegintime',$createbegintime);
            $this->assign('createendtime',$createendtime);
        }else{
            if (strlen($createbegintime)>0){
                $map['TaskCreateTime'] =  array("gt",$createbegintime);
                $this->assign('createbegintime',$createbegintime);
            }
            if (strlen($createendtime)>0){
                $map['TaskCreateTime'] =  array("lt",$createendtime);
                $this->assign('createendtime',$createendtime);
            }
        }

        $assignflag = I('assignflag', '', 'htmlspecialchars');
        if (strlen($assignflag)>0 && $assignflag != "-1"){
            $map['AssignFlag'] = intval($assignflag);
            $this->assign('assignflag',$assignflag);
        }else{
            $this->assign('assignflag',"-1");
        }

        $taskstatus = I('taskstatus', '', 'htmlspecialchars');
        if (strlen($taskstatus)>0 && $taskstatus != "-1"){
            $map['TaskStatus'] = intval($taskstatus);
            $this->assign('taskstatus',$taskstatus);
        }else{
            $this->assign('taskstatus',"-1");
        }

        $taskresult = I('taskresult', '', 'htmlspecialchars');
        if (strlen($taskresult)>0 && $taskresult != "-1"){
            $map['TaskResult'] = intval($taskresult);
            $this->assign('taskresult',$taskresult);
        }else{
            $this->assign('taskresult',"-1");
        }

        $errorcode = I('errorcode', '', 'htmlspecialchars');
        if (strlen($errorcode)>0 && $errorcode != "-1"){
            $map['ErrorCode'] = intval(hexdec($errorcode));
            $this->assign('errorcode',$errorcode);
        }else{
            $this->assign('errorcode',"-1");
        }

        if ($map>0){
            $count = M()->table("ZRVCompressTaskAssignInfo")->where($map)->order('uploadtime desc')->count();
            $Page       = new \Think\Page($count,10);// 实例化分页类 传入总记录数和每页显示的记录数
            $show       = $Page->show();// 分页显示输出
            $this->assign('page',$show);// 赋值分页输出
            $list = M()->table("ZRVCompressTaskAssignInfo")->where($map)->order('uploadtime desc')->limit($Page->firstRow.','.$Page->listRows)->select();
        }else{
            $count = M()->table("ZRVCompressTaskAssignInfo")->order('uploadtime desc')->count();

            $Page       = new \Think\Page($count,10);// 实例化分页类 传入总记录数和每页显示的记录数
            $show       = $Page->show();// 分页显示输出
            $this->assign('page',$show);// 赋值分页输出
            $list = M()->table("ZRVCompressTaskAssignInfo")->order('uploadtime desc')->limit($Page->firstRow.','.$Page->listRows)->select();
        }

        $this->assign("querycount",$count);
        //dump($list);

        for ($i=0;$i<count($list);$i++){
            $date = date("Y-m-d H:i:s",$list[$i]["uploadtime"]);
            $list[$i]["uploadtime"] = $date;

            $s = intval($list[$i]["taskstatus"]);
            switch ($s){
                case 0;
                    $list[$i]["taskstatus"]="未开始压缩任务";
                    break;
                case 1;
                    $list[$i]["taskstatus"]="正在压缩任务";
                    break;
                case 2;
                    $list[$i]["taskstatus"]="压缩完成";
                    break;
                case 3;
                    $list[$i]["taskstatus"]="压缩超时";
                    break;
            }

            $r = intval($list[$i]["taskresult"]);
            switch ($r){
                case 0;
                    $list[$i]["taskresult"]="等待结果";
                    break;
                case 1;
                    $list[$i]["taskresult"]="成功";
                    break;
                case 2;
                    $list[$i]["taskresult"]="失败";
                    break;
            }
        }
        $this->assign("list",$list);
        $this->display();
    }

    public function export2excel(){
        $count = M()->table("ZRVCompressTaskAssignInfo")->order('uploadtime desc')->count();
        $list = M()->table("ZRVCompressTaskAssignInfo")->order('uploadtime desc')->select();
        vendor("PHPExcel.PHPExcel");
        $objPHPExcel = new \PHPExcel();

        // Set document properties
        $objPHPExcel->getProperties()->setCreator("Maarten Balliauw")
            ->setLastModifiedBy("Maarten Balliauw")
            ->setTitle("Office 2007 XLSX Test Document")
            ->setSubject("Office 2007 XLSX Test Document")
            ->setDescription("Test document for Office 2007 XLSX, generated using PHP classes.")
            ->setKeywords("office 2007 openxml php")
            ->setCategory("Test result file");

        $line=1;
        $objPHPExcel->setActiveSheetIndex(0)
            ->setCellValue('A'.$line, "记录编号")
            ->setCellValue('B'.$line, "文件名称")
            ->setCellValue('C'.$line, "文件后缀名称")
            ->setCellValue('D'.$line, "源文件大小")
            ->setCellValue('E'.$line, "上传单位")
            ->setCellValue('F'.$line, "上传时间")
            ->setCellValue('G'.$line, "存储路径")
            ->setCellValue('H'.$line, "压缩后的文件大小")
            ->setCellValue('I'.$line, "压缩后的存储路径")
            ->setCellValue('J'.$line, "分配标示")
            ->setCellValue('K'.$line, "平台IP地址")
            ->setCellValue('L'.$line, "任务来源的平台IP地址")
            ->setCellValue('M'.$line, "压缩任务创建时间")
            ->setCellValue('N'.$line, "ZRV压缩开始时间")
            ->setCellValue('O'.$line, "ZRV压缩结束时间")
            ->setCellValue('P'.$line, "任务状态")
            ->setCellValue('Q'.$line, "任务结果")
            ->setCellValue('R'.$line, "错误码")
            ->setCellValue('S'.$line, "错误原因");

        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('A')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('B')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('C')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('D')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('E')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('F')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('G')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('H')->setWidth(16);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('I')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('J')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('K')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('L')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('M')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('N')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('O')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('P')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('Q')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('R')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('S')->setWidth(36);

        for($i=0;$i<count($list);$i++)
        {
            $line++;

            $date = date("Y-m-d H:i:s",$list[$i]["uploadtime"]);
            $list[$i]["uploadtime"] = $date;
            $date1 = date("Y-m-d H:i:s",$list[$i]["zrvcompressbegintime"]);
            $list[$i]["zrvcompressbegintime"] = $date1;
            $date2 = date("Y-m-d H:i:s",$list[$i]["zrvcompressendtime"]);
            $list[$i]["zrvcompressendtime"] = $date2;

            if ($list[$i]["assignflag"] == '0'){
                $list[$i]["assignflag"] = "未分配";
            }else{
                $list[$i]["assignflag"] = "已分配";
            }

            $errcode = intval($list[$i]["errorcode"]);
            $errorreason = "";
            switch ($errcode){
               case 0x01001;
                    $errorreason="云创平台登录失败";
                    break;
                case 0x01002;
                    $errorreason="获取源视频文目录失败";
                    break;
                case 0x01003;
                    $errorreason="获取源视频文件失败";
                    break;
                case 0x01004;
                    $errorreason="获取源视频文件大小失败";
                    break;
                case 0x01005;
                    $errorreason="压缩后文件写入到新目录失败";
                    break;
                case 0x01006;
                    $errorreason="压缩源视频文件失败";
                    break;
                case 0x01007;
                    $errorreason="源视频文件格式不支持";
                    break;
                case 0x01008;
                    $errorreason="分析源视频文件内容失败";
                    break;
                case 0x01009;
                    $errorreason="删除临时文件失败";
                    break;
                case 0x02000;
                    $errorreason="压缩任务结果未返回";
                    break;
            }

            $list[$i]["errorreason"] = $errorreason;
            
            $errorcode_hex = sprintf("%x",intval($list[$i]["errorcode"]));
            $list[$i]["errorcode"] = "0x".$errorcode_hex;

            $s = intval($list[$i]["taskstatus"]);
            switch ($s){
                case 0;
                    $list[$i]["taskstatus"]="未开始压缩任务";
                    break;
                case 1;
                    $list[$i]["taskstatus"]="正在压缩任务";
                    break;
                case 2;
                    $list[$i]["taskstatus"]="压缩完成";
                    break;
                case 3;
                    $list[$i]["taskstatus"]="压缩超时";
                    break;
            }

            $r = intval($list[$i]["taskresult"]);
            switch ($r){
                case 0;
                    $list[$i]["taskresult"]="等待结果";
                    break;
                case 1;
                    $list[$i]["taskresult"]="成功";
                    break;
                case 2;
                    $list[$i]["taskresult"]="失败";
                    break;
            }

            $objPHPExcel->setActiveSheetIndex(0)
                ->setCellValue('A'.$line, $list[$i]["recordnum"])
                ->setCellValue('B'.$line, $list[$i]["filename"])
                ->setCellValue('C'.$line, $list[$i]["filesuffixname"])
                ->setCellValue('D'.$line, $list[$i]["filesize"])
                ->setCellValueExplicit('E'.$line, $list[$i]["uploadunit"],\PHPExcel_Cell_DataType::TYPE_STRING)
                ->setCellValue('F'.$line, $list[$i]["uploadtime"])
                ->setCellValue('G'.$line, $list[$i]["storagepath"])
                ->setCellValue('H'.$line, $list[$i]["yshfilesize"])
                ->setCellValue('I'.$line, $list[$i]["yshstoragepath"])
                ->setCellValue('J'.$line, $list[$i]["assignflag"])
                ->setCellValue('K'.$line, $list[$i]["platformip"])
                ->setCellValue('L'.$line, $list[$i]["zrvdeviceip"])
                ->setCellValue('M'.$line, $list[$i]["taskcreatetime"])
                ->setCellValue('N'.$line, $list[$i]["zrvcompressbegintime"])
                ->setCellValue('O'.$line, $list[$i]["zrvcompressendtime"])
                ->setCellValue('P'.$line, $list[$i]["taskstatus"])
                ->setCellValue('Q'.$line, $list[$i]["taskresult"])
                ->setCellValue('R'.$line, $list[$i]["errorcode"])
                ->setCellValue('S'.$line, $list[$i]["errorreason"]);
        }

        // Rename worksheet
        $objPHPExcel->getActiveSheet()->setTitle('Simple');

        // Set active sheet index to the first sheet, so Excel opens this as the first sheet
        $objPHPExcel->setActiveSheetIndex(0);

        $filname = "taskassign_".time().".xls";

        // Redirect output to a client’s web browser (Excel5)
        header('Content-Type: application/vnd.ms-excel');
        header('Content-Disposition: attachment;filename="'.$filname.'"');
        header('Cache-Control: max-age=0');
        // If you're serving to IE 9, then the following may be needed
        header('Cache-Control: max-age=1');

        // If you're serving to IE over SSL, then the following may be needed
        header ('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
        header ('Last-Modified: '.gmdate('D, d M Y H:i:s').' GMT'); // always modified
        header ('Cache-Control: cache, must-revalidate'); // HTTP/1.1
        header ('Pragma: public'); // HTTP/1.0

        $objWriter = \PHPExcel_IOFactory::createWriter($objPHPExcel, 'Excel5');
        $objWriter->save('php://output');
        exit;
    }

    public function GetTaskAssignDetail(){
        $recordnum=I('recordnum', "", 'htmlspecialchars');
        $info = M()->table("ZRVCompressTaskAssignInfo")->where(array('RecordNum'=>$recordnum))->find();
        //dump($info);

        if ($info){
            $taskstatus = intval($info["taskstatus"]);
            if ($taskstatus == 1)
                $info["taskstatus"]="正在压缩任务";
            if ($taskstatus == 2)
                $info["taskstatus"]="压缩完成";
            if ($taskstatus == 3)
                $info["taskstatus"]="压缩超时";

            $taskresult = intval($info["taskresult"]);
            if ($taskresult == 0)
                $info["taskresult"] = "等待结果";
            if ($taskresult == 1)
                $info["taskresult"] = "成功";
            if ($taskresult == 2)
                $info["taskresult"] = "失败";

            $assignflag = intval($info["assignflag"]);
            if ($assignflag == 0){
                $info["assignflag"] = "未分配";
            }else{
                $info["assignflag"] = "已分配";
            }

            $errcode = intval($info["errorcode"]);
            $errorreason = "";
            switch ($errcode){
               case 0x01001;
                    $errorreason="云创平台登录失败";
                    break;
                case 0x01002;
                    $errorreason="获取源视频文目录失败";
                    break;
                case 0x01003;
                    $errorreason="获取源视频文件失败";
                    break;
                case 0x01004;
                    $errorreason="获取源视频文件大小失败";
                    break;
                case 0x01005;
                    $errorreason="压缩后文件写入到新目录失败";
                    break;
                case 0x01006;
                    $errorreason="压缩源视频文件失败";
                    break;
                case 0x01007;
                    $errorreason="源视频文件格式不支持";
                    break;
                case 0x01008;
                    $errorreason="分析源视频文件内容失败";
                    break;
                case 0x01009;
                    $errorreason="删除临时文件失败";
                    break;
                case 0x02000;
                    $errorreason="压缩任务结果未返回";
                    break;
            }
            $info["errorreason"] ="错误码: 0x".sprintf("%x",$errcode)." 错误原因:".$errorreason;

            if ($taskresult != 2)
            {
                $info["errorreason"] ="无";
            }

            $date1 = date("Y-m-d H:i:s",$info["uploadtime"]);
            $info["uploadtime"] = $date1;

            $date2 = date("Y-m-d H:i:s",$info["zrvcompressbegintime"]);
            $info["zrvcompressbegintime"] = $date2;

            $date3 = date("Y-m-d H:i:s",$info["zrvcompressendtime"]);
            $info["zrvcompressendtime"] = $date3;

            $filesize1 = intval($info["filesize"]);
            $filesize2 = number_format($filesize1).' Byte';
            $info["filesize"] = $filesize2;
            
            $yshfilesize1 = intval($info["yshfilesize"]);
            $yshfilesize2 = number_format($yshfilesize1).' Byte';
            $info["yshfilesize"] = $yshfilesize2;   
            //dump($info);
        }

        //$this->assign("info",$info);
        $this->ajaxReturn($info);
        //$this->display('Taskassign/detail');
    }
    
    //public function detail($recordnum){
    //    $info = M()->table("ZRVCompressTaskAssignInfo")->where(array('RecordNum'=>$recordnum))->find();
    //    //dump($info);

    //    if ($info){
    //        $taskstatus = intval($info["taskstatus"]);
    //        if ($taskstatus == 1)
    //            $info["taskstatus"]="正在压缩任务";
    //        if ($taskstatus == 2)
    //            $info["taskstatus"]="压缩完成";
    //        if ($taskstatus == 3)
    //            $info["taskstatus"]="压缩超时";


    //        $taskresult = intval($info["taskresult"]);
    //        if ($taskresult == 0)
    //            $info["taskresult"] = "等待结果";
    //        if ($taskresult == 1)
    //            $info["taskresult"] = "成功";
    //        if ($taskresult == 2)
    //            $info["taskresult"] = "失败";


    //        $assignflag = intval($info["assignflag"]);
    //        if ($assignflag == 0){
    //            $info["assignflag"] = "未分配";
    //        }else{
    //            $info["assignflag"] = "已分配";
    //        }

    //        $errcode = intval($info["errorcode"]);
    //        $errorreason = "";
    //        switch ($errcode){
    //            case 0x01001;
    //                $errorreason="云创平台登录失败";
    //                break;
    //            case 0x01002;
    //                $errorreason="源视频文件格式不支持";
    //                break;
    //            case 0x01003;
    //                $errorreason="源视频文件压缩失败";
    //                break;
    //            case 0x01004;
    //                $errorreason="获取源视频文件目录错误";
    //                break;
    //            case 0x01005;
    //                $errorreason="获取源视频文件失败";
    //                break;
    //            case 0x01006;
    //                $errorreason="获取源视频文件大小失败";
    //                break;
    //            case 0x01007;
    //                $errorreason="源视频文件格式不支持";
    //                break;
    //            case 0x01008;
    //                $errorreason="分析源视频文件内容失败";
    //                break;
    //            case 0x01009;
    //                $errorreason="删除临时文件失败";
    //                break;
    //            case 0x02000;
    //                $errorreason="压缩任务结果未返回";
    //                break;
    //        }
    //        $info["errorreason"] ="错误码: 0x".sprintf("%x",$errcode)." 错误原因:".$errorreason;


    //        $date1 = date("Y-m-d H:i:s",$info["uploadtime"]);
    //        $info["uploadtime"] = $date1;

    //        $date2 = date("Y-m-d H:i:s",$info["zrvcompressbegintime"]);
    //        $info["zrvcompressbegintime"] = $date2;

    //        $date3 = date("Y-m-d H:i:s",$info["zrvcompressendtime"]);
    //        $info["zrvcompressendtime"] = $date3;


    //        //dump($info);
    //    }

    //    $this->assign("info",$info);
    //    $this->display('Taskassign/detail');
    //}

    public function GetTaskAssign(){
        $pagenow = I('pagenow', 1, 'htmlspecialchars');
        $pagesize = I('pagesize', 10, 'htmlspecialchars');
        $pagetype=I('pagetype', "", 'htmlspecialchars');
        //$count = M()->table("ZRVCompressTaskAssignInfo")->count();
        $count=M()->table("ZRVCompressTaskAssignInfo")->join("ZRVDeviceInfo ON ZRVCompressTaskAssignInfo.ZRVDeviceIP=ZRVDeviceInfo.DeviceIP")->count();        
        FenYeController::Instance()->rowCount=$count;
        FenYeController::Instance()->pageNow=$pagenow;
        FenYeController::Instance()->pageSize=$pagesize;
        $pagecount=ceil(FenYeController::Instance()->rowCount/FenYeController::Instance()->pageSize);
        FenYeController::Instance()->pageCount=$pagecount;
        $start=(FenYeController::Instance()->pageNow-1) * FenYeController::Instance()->pageSize;
        $end=FenYeController::Instance()->pageSize;
        $ZRVDeviceInfo = M()->table("ZRVCompressTaskAssignInfo")->field('recordnum,filename,uploadtime,taskcreatetime,zrvdeviceip,taskstatus')->order("uploadtime desc")->limit($start,$end)->select();
        
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
                
                
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["recordnum"]."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["filename"]."</td>";
                $ZRVDeviceStr.="<td>".date('Y-m-d H:i:s',$ZRVDeviceInfo[$i]["uploadtime"])."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["taskcreatetime"]."</td>";
                $ZRVDeviceStr.="<td>".$ZRVDeviceInfo[$i]["zrvdeviceip"]."</td>";
                
                if(!empty($pagetype)){
                    FenYeController::Instance()->funccall="setPage2";
                    $ZRVDeviceStr.="<td><a href='javascript:void(0);' onclick=gettaskdetail2('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a></td>";
                }else{
                    FenYeController::Instance()->funccall="setPage";
                    $ZRVDeviceStr.="<td><a href='javascript:void(0);' onclick=gettaskdetail('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a></td>";
                }
                
                //$ZRVDeviceStr.="<td><a href='javascript:void(0);' onclick=gettaskdetail('".$ZRVDeviceInfo[$i]["recordnum"]."')>详情</a></td>";
                $ZRVDeviceStr.="</tr>";
            }
        }
        echo $ZRVDeviceStr."<tr><td colspan=6 style='text-align:right;'>".FenYeController::Instance()->getNavigate2()."</td></tr>";
    }
}