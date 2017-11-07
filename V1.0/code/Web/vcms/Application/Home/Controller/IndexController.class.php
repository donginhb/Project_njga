<?php
namespace Home\Controller;
use Think\Controller;
class IndexController extends Controller {
    public function index(){

        //$this->redirect('Platform/index',0);
        $this->redirect('Task/index',0);
        //$this->display();
    }
}