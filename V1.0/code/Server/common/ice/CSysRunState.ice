//QUERY_SYS_RUN_STATE
module WiscomEV9000
{
	sequence<byte>  StructBuffer;
	interface CSysRunState
	{
		int SysRunState(string strInData,out string strOutData);
	}; 
};