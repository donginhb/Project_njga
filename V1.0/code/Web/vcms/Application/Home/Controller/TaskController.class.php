<?php
namespace Home\Controller;
use Think\Controller;

class TaskController extends Controller {
    public function index(){
       // 查询条件处理  
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
        
        //压缩任务开始时间
        $zrvbegin_begintime = I('zrvbegin_begintime', '', 'htmlspecialchars');
        $zrvbegin_endtime = I('zrvbegin_endtime', '', 'htmlspecialchars');
        if(strlen($zrvbegin_begintime)>0 && $zrvbegin_begintime != "NaN" && strlen($zrvbegin_endtime)>0 && $zrvbegin_endtime != "NaN"){
            $map['ZRVCompressBeginTime'] =  array(array("gt",intval($zrvbegin_begintime)),array("lt",intval($zrvbegin_endtime)));
            $zrvbegin_begintime_str = date("Y-m-d H:i:s",intval($zrvbegin_begintime));
            $this->assign('zrvbegin_begintime_str',$zrvbegin_begintime_str);
            $zrvbegin_endtime_str = date("Y-m-d H:i:s",intval($zrvbegin_endtime));
            $this->assign('zrvbegin_endtime_str',$zrvbegin_endtime_str);
        }else{
            if (strlen($zrvbegin_begintime)>0 && $zrvbegin_begintime != "NaN"){
                $map['ZRVCompressBeginTime'] =  array("gt",intval($zrvbegin_begintime));

                $zrvbegin_begintime_str = date("Y-m-d H:i:s",intval($zrvbegin_begintime));
                $this->assign('zrvbegin_begintime_str',$zrvbegin_begintime_str);
            }
            if (strlen($zrvbegin_endtime)>0 && $zrvbegin_endtime != "NaN"){
                $map['ZRVCompressBeginTime'] =  array("lt",intval($zrvbegin_endtime));
                $zrvbegin_endtime_str = date("Y-m-d H:i:s",intval($zrvbegin_endtime));
                $this->assign('zrvbegin_endtime_str',$zrvbegin_endtime_str);
            }
        }
        
        //压缩任务结束时间
        $zrvend_begintime = I('zrvend_begintime', '', 'htmlspecialchars');
        $zrvend_endtime = I('zrvend_endtime', '', 'htmlspecialchars');
        if(strlen($zrvend_begintime)>0 && $zrvend_begintime != "NaN" && strlen($zrvend_endtime)>0 && $zrvend_endtime != "NaN"){
            $map['ZRVCompressEndTime'] =  array(array("gt",intval($zrvend_begintime)),array("lt",intval($zrvend_endtime)));
            $zrvend_begintime_str = date("Y-m-d H:i:s",intval($zrvend_begintime));
            $this->assign('zrvend_begintime_str',$zrvend_begintime_str);
            $zrvend_endtime_str = date("Y-m-d H:i:s",intval($zrvend_endtime));
            $this->assign('zrvend_endtime_str',$zrvend_endtime_str);
        }else{
            if (strlen($zrvend_begintime)>0 && $zrvend_begintime != "NaN"){
                $map['ZRVCompressEndTime'] =  array("gt",intval($zrvend_begintime));

                $zrvend_begintime_str = date("Y-m-d H:i:s",intval($zrvend_begintime));
                $this->assign('zrvend_begintime_str',$zrvend_begintime_str);
            }
            if (strlen($zrvend_endtime)>0 && $zrvend_endtime != "NaN"){
                $map['ZRVCompressEndTime'] =  array("lt",intval($zrvend_endtime));
                $zrvend_endtime_str = date("Y-m-d H:i:s",intval($zrvend_endtime));
                $this->assign('zrvend_endtime_str',$zrvend_endtime_str);
            }
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
            $count = M()->table("ZRVCompressTaskInfo")->where($map)->order('uploadtime desc')->count();
            $Page       = new \Think\Page($count,10);// 实例化分页类 传入总记录数和每页显示的记录数
            $show       = $Page->show();// 分页显示输出
            $this->assign('page',$show);// 赋值分页输出
            $list = M()->table("ZRVCompressTaskInfo")->where($map)->order('uploadtime desc')->limit($Page->firstRow.','.$Page->listRows)->select();
        }else{
            $count = M()->table("ZRVCompressTaskInfo")->order('uploadtime desc')->count();
            $Page       = new \Think\Page($count,10);// 实例化分页类 传入总记录数和每页显示的记录数
            $show       = $Page->show();// 分页显示输出
            $this->assign('page',$show);// 赋值分页输出
            $list = M()->table("ZRVCompressTaskInfo")->order('uploadtime desc')->limit($Page->firstRow.','.$Page->listRows)->select();
        }
        $allres=M()->table("ZRVCompressTaskInfo")->field('errorcode')->where($map)->select();
        $this->assign("querycount",$count);        
        //dump($list);
        $failnum=0;
        $successnum=0;

        $failnum=0;
        $successnum=0;
        $error1num=0;
        $error2num=0;
        $error3num=0;
        $error4num=0;
        $error5num=0;
        $error6num=0;
        $error7num=0;
        $error8num=0;
        $error9num=0;
        $error10num=0;
        
        for ($i=0;$i<count($list);$i++){
            $date = date("Y-m-d H:i:s",$list[$i]["uploadtime"]);
            $list[$i]["uploadtime"] = $date;
            
            $date1 = date("Y-m-d H:i:s",$list[$i]["zrvcompressbegintime"]);
            $list[$i]["zrvcompressbegintime"] = $date1; 

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
        
        for($j=0;$j<count($allres);$j++){
            $ec = intval($allres[$j]["errorcode"]);
            switch($ec){
                case 0x01001:
                    $failnum++;
                    $error1num++;
                    break;
                case 0x01002:
                    $failnum++;
                    $error2num++;
                    break;
                case 0x01003:
                    $failnum++;
                    $error3num++;
                    break;
                case 0x01004:
                    $failnum++;
                    $error4num++;
                    break;
                case 0x01005:
                    $failnum++;
                    $error5num++;
                    break;
                case 0x01006:
                    $failnum++;
                    $error6num++;
                    break;
                case 0x01007:
                    $failnum++;
                    $error7num++;
                    break;
                case 0x01008:
                    $failnum++;
                    $error8num++;
                    break;
                case 0x01009:
                    $failnum++;
                    $error9num++;
                    break;
                case 0x02000:
                    $failnum++;
                    $error10num++;
                    break;    
                case 200:
                    $successnum++;
                    break;
            }
        }
        
        $allres['failnum']=$failnum;
        $allres['successnum']=$successnum;
        $allres["error1num"]=$error1num;
        $allres["error2num"]=$error2num;
        $allres["error3num"]=$error3num;
        $allres["error4num"]=$error4num;
        $allres["error5num"]=$error5num;
        $allres["error6num"]=$error6num;
        $allres["error7num"]=$error7num;
        $allres["error8num"]=$error8num;
        $allres["error9num"]=$error9num;
        $allres["error10num"]=$error10num;

        $this->assign("list",$list);
        $this->assign("allres",$allres);

        $this->display();
    }

    public function export2excel(){

        $deviceip = I('deviceip', '', 'htmlspecialchars');
        if (strlen($deviceip)>0){
            $map['ZRVDeviceIP'] = $deviceip;
        }

        $begintime = I('begintime', '', 'htmlspecialchars');
        $endtime = I('endtime', '', 'htmlspecialchars');
        if(strlen($begintime)>0 && $begintime != "NaN" && strlen($endtime)>0 && $endtime != "NaN"){
            $map['UploadTime'] =  array(array("gt",intval($begintime)),array("lt",intval($endtime)));
        }else{
            if (strlen($begintime)>0 && $begintime != "NaN"){
                $map['UploadTime'] =  array("gt",intval($begintime));
            }
            if (strlen($endtime)>0 && $endtime != "NaN"){
                $map['UploadTime'] =  array("lt",intval($endtime));
            }
        }

        // 任务创建时间处理
        $createbegintime = I('createbegintime', '', 'htmlspecialchars');
        $createendtime = I('createendtime', '', 'htmlspecialchars');
        if(strlen($createbegintime)>0 && strlen($createendtime)>0){
            $map['TaskCreateTime'] =  array(array("gt",$createbegintime),array("lt",$createendtime));
        }else{
            if (strlen($createbegintime)>0){
                $map['TaskCreateTime'] =  array("gt",$createbegintime);
            }
            if (strlen($createendtime)>0){
                $map['TaskCreateTime'] =  array("lt",$createendtime);
            }
        }

        //压缩任务开始时间
        $zrvbegin_begintime = I('zrvbegin_begintime', '', 'htmlspecialchars');
        $zrvbegin_endtime = I('zrvbegin_endtime', '', 'htmlspecialchars');
        if(strlen($zrvbegin_begintime)>0 && $zrvbegin_begintime != "NaN" && strlen($zrvbegin_endtime)>0 && $zrvbegin_endtime != "NaN"){
            $map['ZRVCompressBeginTime'] =  array(array("gt",intval($zrvbegin_begintime)),array("lt",intval($zrvbegin_endtime)));
            $zrvbegin_begintime_str = date("Y-m-d H:i:s",intval($zrvbegin_begintime));
            $this->assign('zrvbegin_begintime_str',$zrvbegin_begintime_str);
            $zrvbegin_endtime_str = date("Y-m-d H:i:s",intval($zrvbegin_endtime));
            $this->assign('zrvbegin_endtime_str',$zrvbegin_endtime_str);
        }else{
            if (strlen($zrvbegin_begintime)>0 && $zrvbegin_begintime != "NaN"){
                $map['ZRVCompressBeginTime'] =  array("gt",intval($zrvbegin_begintime));

                $zrvbegin_begintime_str = date("Y-m-d H:i:s",intval($zrvbegin_begintime));
                $this->assign('zrvbegin_begintime_str',$zrvbegin_begintime_str);
            }
            if (strlen($zrvbegin_endtime)>0 && $zrvbegin_endtime != "NaN"){
                $map['ZRVCompressBeginTime'] =  array("lt",intval($zrvbegin_endtime));
                $zrvbegin_endtime_str = date("Y-m-d H:i:s",intval($zrvbegin_endtime));
                $this->assign('zrvbegin_endtime_str',$zrvbegin_endtime_str);
            }
        }
        
        //压缩任务结束时间
        $zrvend_begintime = I('zrvend_begintime', '', 'htmlspecialchars');
        $zrvend_endtime = I('zrvend_endtime', '', 'htmlspecialchars');
        if(strlen($zrvend_begintime)>0 && $zrvend_begintime != "NaN" && strlen($zrvend_endtime)>0 && $zrvend_endtime != "NaN"){
            $map['ZRVCompressEndTime'] =  array(array("gt",intval($zrvend_begintime)),array("lt",intval($zrvend_endtime)));
            $zrvend_begintime_str = date("Y-m-d H:i:s",intval($zrvend_begintime));
            $this->assign('zrvend_begintime_str',$zrvend_begintime_str);
            $zrvend_endtime_str = date("Y-m-d H:i:s",intval($zrvend_endtime));
            $this->assign('zrvend_endtime_str',$zrvend_endtime_str);
        }else{
            if (strlen($zrvend_begintime)>0 && $zrvend_begintime != "NaN"){
                $map['ZRVCompressEndTime'] =  array("gt",intval($zrvend_begintime));

                $zrvend_begintime_str = date("Y-m-d H:i:s",intval($zrvend_begintime));
                $this->assign('zrvend_begintime_str',$zrvend_begintime_str);
            }
            if (strlen($zrvend_endtime)>0 && $zrvend_endtime != "NaN"){
                $map['ZRVCompressEndTime'] =  array("lt",intval($zrvend_endtime));
                $zrvend_endtime_str = date("Y-m-d H:i:s",intval($zrvend_endtime));
                $this->assign('zrvend_endtime_str',$zrvend_endtime_str);
            }
        }

        $taskresult = I('taskresult', '', 'htmlspecialchars');
        if (strlen($taskresult)>0 && $taskresult != "-1"){
            $map['TaskResult'] = intval($taskresult);
        }

        $errorcode = I('errorcode', '', 'htmlspecialchars');
        if (strlen($errorcode)>0 && $errorcode != "-1"){
            $map['ErrorCode'] = intval(hexdec($errorcode));
        }

        if ($map>0){
            $count = M()->table("ZRVCompressTaskInfo")->where($map)->order('uploadtime desc')->count();

            $list = M()->table("ZRVCompressTaskInfo")->where($map)->order('uploadtime desc')->select();
        }else{
            $count = M()->table("ZRVCompressTaskInfo")->order('uploadtime desc')->count();
            $list = M()->table("ZRVCompressTaskInfo")->order('uploadtime desc')->select();
        }

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
            ->setCellValue('J'.$line, "平台IP地址")
            ->setCellValue('K'.$line, "任务来源的平台IP地址")
            ->setCellValue('L'.$line, "压缩任务创建时间")
            ->setCellValue('M'.$line, "ZRV压缩开始时间")
            ->setCellValue('N'.$line, "ZRV压缩结束时间")
            ->setCellValue('O'.$line, "任务状态")
            ->setCellValue('P'.$line, "任务结果")
            ->setCellValue('Q'.$line, "错误码")
            ->setCellValue('R'.$line, "错误原因");

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
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('K')->setWidth(36);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('L')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('M')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('N')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('O')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('P')->setWidth(12);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('Q')->setAutoSize(true);
        $objPHPExcel->setActiveSheetIndex(0)->getColumnDimension('R')->setWidth(36);

        for($i=0;$i<count($list);$i++)
        {
            $line++;

            $date = date("Y-m-d H:i:s",$list[$i]["uploadtime"]);
            $list[$i]["uploadtime"] = $date;
            $date1 = date("Y-m-d H:i:s",$list[$i]["zrvcompressbegintime"]);
            $list[$i]["zrvcompressbegintime"] = $date1;
            $date2 = date("Y-m-d H:i:s",$list[$i]["zrvcompressendtime"]);
            $list[$i]["zrvcompressendtime"] = $date2;

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
                ->setCellValue('J'.$line, $list[$i]["platformip"])
                ->setCellValue('K'.$line, $list[$i]["zrvdeviceip"])
                ->setCellValue('L'.$line, $list[$i]["taskcreatetime"])
                ->setCellValue('M'.$line, $list[$i]["zrvcompressbegintime"])
                ->setCellValue('N'.$line, $list[$i]["zrvcompressendtime"])
                ->setCellValue('O'.$line, $list[$i]["taskstatus"])
                ->setCellValue('P'.$line, $list[$i]["taskresult"])
                ->setCellValue('Q'.$line, $list[$i]["errorcode"])
                ->setCellValue('R'.$line, $list[$i]["errorreason"]);
        }

        // Rename worksheet
        $objPHPExcel->getActiveSheet()->setTitle('Simple');

        // Set active sheet index to the first sheet, so Excel opens this as the first sheet
        $objPHPExcel->setActiveSheetIndex(0);

        $filname = "task_".time().".xls";

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

    public function detail($recordnum){

        $info = M()->table("ZRVCompressTaskInfo")->where(array('RecordNum'=>$recordnum))->find();

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

            $date1 = date("Y-m-d H:i:s",$info["uploadtime"]);
            $info["uploadtime"] = $date1;

            $date2 = date("Y-m-d H:i:s",$info["zrvcompressbegintime"]);
            $info["zrvcompressbegintime"] = $date2;

            $date3 = date("Y-m-d H:i:s",$info["zrvcompressendtime"]);
            $info["zrvcompressendtime"] = $date3;
            
            $filesize1 = intval($info["filesize"]);
            $filesize2 = number_format($filesize1);
            $info["filesize"] = $filesize2;
            
            $yshfilesize1 = intval($info["yshfilesize"]);
            $yshfilesize2 = number_format($yshfilesize1);
            $info["yshfilesize"] = $yshfilesize2;   
        }

        $this->assign("info",$info);
        $this->display('Taskassign/detail');
    }

    public function GetCompressTaskQuery(){
        $recordnum = I('recordnum', '', 'htmlspecialchars');
        $info = M()->table("ZRVCompressTaskInfo")->where(array('RecordNum'=>$recordnum))->find();

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
            $filesize2 = number_format($filesize1);
            $info["filesize"] = $filesize2;
            
            $yshfilesize1 = intval($info["yshfilesize"]);
            $yshfilesize2 = number_format($yshfilesize1);
            $info["yshfilesize"] = $yshfilesize2;            
        }

        //$this->assign("info",$info);
        //$this->display('Taskassign/detail');
        $this->ajaxReturn($info);
    }
}