#!/usr/bin/env ruby

require 'time'
require 'bigdecimal'
require "httpclient"
require "json"
require 'mysql2'
require 'rufus-scheduler'

$my_yoyow_check_config = {
  "head_age_threshold" => 15,              #seconds
  "participation_rate_threshold" => 79.999 #percent
}
$my_exchange_config = {
  "yoyow_account" => "123456789", 
  "yoyow_asset_id" => 0,
  "mysql_config" => {
    "host" => "192.168.1.1",
    "user" => "exchange",
    "pass" => "Yoyow12345",
    "db"   => "exchange"
  }
}
def my_yoyow_config
  {
    "uri"  => 'http://127.0.0.1:8091/rpc',
    "user" => "",
    "pass" => ""
  }
end

def yoyow_post (data:{}, timeout:55)
  if data.nil? or data.empty?
    return
  end

  client = HTTPClient.new
  client.connect_timeout=timeout
  client.receive_timeout=timeout

  myconfig = my_yoyow_config
  uri  = myconfig["uri"]
  user = myconfig["user"]
  pass = myconfig["pass"]

  client.set_auth uri, user, pass

  begin
    response = client.post uri, data.to_json, nil
    response_content = response.content
    response_json = JSON.parse response_content
    return response_json
  rescue Exception => e
    print "yoyow_post error: "
    puts e
  end
end

# params is an array
def yoyow_command (command:nil, params:nil, timeout:55)
  if command.nil? or params.nil?
    return
  end
  data = {
    "jsonrpc" => "2.0",
    "method"  => command,
    "params"  => params,
    "id"      => 0
  }
  return yoyow_post data:data, timeout:timeout
end

#main
if __FILE__ == $0

  logger = Logger.new(STDOUT)
  gwcf = $my_exchange_config
  sqlcf = gwcf["mysql_config"]
  sqlc = Mysql2::Client.new(:host=>sqlcf["host"],:username=>sqlcf["user"],:password=>sqlcf["pass"],:database=>sqlcf["db"])
  yoyow_account = gwcf["yoyow_account"]
  yoyow_asset_id = gwcf["yoyow_asset_id"]

  # run
  scheduler = Rufus::Scheduler.new
  scheduler.every '10', first: :now, overlap: false do
   begin
    # [YOYOW] check if current node or current network status is ok
    r = yoyow_command command:"is_locked", params:[]
    if r["result"] == true
      msg = "[YOYOW] Warning: wallet is locked."
      logger.warn msg
      next
    end

    r = yoyow_command command:"info", params:[]
    yoyow_head_num = r["result"]["head_block_num"]
    yoyow_head_time = r["result"]["head_block_time"]
    yoyow_head_age_sec = (Time.now.utc - Time.parse(yoyow_head_time + 'Z')).to_i
    yoyow_lib = r["result"]["last_irreversible_block_num"]
    yoyow_participation_rate = BigDecimal.new(r["result"]["participation"])

    logger.info "[YOYOW] head block #%d, LIB #%d, time %s, age %d s, NPR %.2f %" %
                 [ yoyow_head_num, yoyow_lib, yoyow_head_time, yoyow_head_age_sec, yoyow_participation_rate.to_f ]
    if yoyow_head_age_sec >= $my_yoyow_check_config["head_age_threshold"]
      msg = "[YOYOW] Warning: head block age %d seconds." % yoyow_head_age_sec
      logger.warn msg
      next
    end
    if yoyow_participation_rate <= $my_yoyow_check_config["participation_rate_threshold"]
      msg = "[YOYOW] Warning: participation rate %.2f %" % yoyow_participation_rate.to_f
      logger.warn msg
      next
    end

    # [SQL] reconnect
    if sqlc.nil? or sqlc.closed?
      sqlc = Mysql2::Client.new(:host=>sqlcf["host"],:username=>sqlcf["user"],:password=>sqlcf["pass"],:database=>sqlcf["db"])
    end

    # [YOYOW] get last processed number
    yoyow_next_seq = 1
    sql = "SELECT next_seq_no FROM yoyow_info WHERE monitor_account = " + yoyow_account
    sqlr = sqlc.query(sql)
    if sqlr.size == 0
      sqlc.query("insert into yoyow_info(monitor_account,next_seq_no) values(" + yoyow_account + ",1)")
    else
      sqlr.each do |row|
        # do something with row, it's ready to rock
        yoyow_next_seq = row["next_seq_no"]
      end
    end
    logger.info "[YOYOW] next seq to process = %d" % yoyow_next_seq

    # [YOYOW] process new history
    sql = "insert into yoyow_his(monitor_account,seq_no,from_account,to_account,amount,asset_id," +
                              "decrypted_memo,description,block_num,block_time,trx_in_block,op_in_trx,virtual_op,trx_id," +
                              "process_status) " +
                              "values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
    stmt = sqlc.prepare(sql)
    yoyow_last_block_num = 0
    yoyow_last_block = nil
    yoyow_max_seq = 0
    r = yoyow_command command:"get_relative_account_history", params:[yoyow_account,0,0,1,0]
    if not r["result"].empty?
      yoyow_max_seq = r["result"][0]["sequence"]
    end
    r = yoyow_command command:"get_relative_account_history", params:[yoyow_account,0,yoyow_next_seq,10,yoyow_next_seq+9]
    catch :reached_lib do
     while yoyow_next_seq <= yoyow_max_seq do
      r["result"].reverse.each{ |rec|
         seq_no = rec["sequence"]
         block_num = rec["op"]["block_num"]
         throw :reached_lib if block_num >= yoyow_lib # don't process records later than LIB
         yoyow_next_seq = seq_no + 1
         next if rec["op"]["op"][0] != 0 # not transfer
         logger.info "[YOYOW]" + rec.to_s
         sqlr = sqlc.query("select 1 from yoyow_his where monitor_account = " + yoyow_account + " and seq_no = " + seq_no.to_s)
         if sqlr.size > 0
           logger.info "[YOYOW] skipping because [account,seq] = [%s,%d] is already in db" % [ yoyow_account, seq_no ]
           next
         end
         if block_num != yoyow_last_block_num
           yoyow_last_block_num = block_num
           r1 = yoyow_command command:"get_block", params:[block_num]
           yoyow_last_block = r1["result"]
         end
         from_account = rec["op"]["op"][1]["from"].to_s
         to_account = rec["op"]["op"][1]["to"].to_s
         amount = rec["op"]["op"][1]["amount"]["amount"].to_i
         asset_id = rec["op"]["op"][1]["amount"]["asset_id"]
         decrypted_memo = rec["memo"]
         description = rec["description"]
         block_time = rec["op"]["block_timestamp"]
         trx_in_block = rec["op"]["trx_in_block"]
         op_in_trx = rec["op"]["op_in_trx"]
         virtual_op = rec["op"]["virtual_op"]
         trx_id = yoyow_last_block["transaction_ids"][trx_in_block]
         process_status = 0 # new
         if from_account == yoyow_account
           process_status = 2 # out
           sqlc.query("update withdraw_his set process_status = 23, out_block_num = " + block_num.to_s +
                             " where out_platform = 'yoyow' " +
                             "and out_trx_id = '" + trx_id + "' and process_status = 22")
         elsif asset_id != yoyow_asset_id
           process_status = 101 # bad asset
         elsif decrypted_memo.nil? or decrypted_memo.length == 0
           process_status = 102 # empty memo
         else
           # parse memo
           memo_to_parse = decrypted_memo
           # check if the memo is valid
           if memo_to_parse.ok?
               process_status = 11 # good memo
               # to process
           else
               process_status = 104 # bad memo
           end
         end
         stmt.execute(yoyow_account,seq_no,from_account.to_i,to_account.to_i,amount,asset_id,
                      decrypted_memo,description,block_num,block_time,trx_in_block,op_in_trx,virtual_op,trx_id,
                      process_status)
      }
      r = yoyow_command command:"get_relative_account_history", params:[yoyow_account,0,yoyow_next_seq,10,yoyow_next_seq+9]
     end # while
    end #catch
    sqlc.query("update yoyow_info set next_seq_no = " + yoyow_next_seq.to_s + " where monitor_account = " + yoyow_account)

    # [SQL] process new withdrawals
    sql = "select * from withdraw_his where process_status = 11 order by seq_no"
    sqlr = sqlc.query(sql)
    if sqlr.size > 0
     catch :collected_csaf do
      # [YOYOW] check account statistics
      r = yoyow_command command:"get_full_account", params:[yoyow_account]
      stats = r["result"]["statistics"]
      if stats["csaf"] < 2000000 # 20 YOYO
        logger.info "[YOYOW] collecting CSAF"
        r1 = yoyow_command command:"collect_csaf", params:[yoyow_account,yoyow_account,20,"YOYO",true]
        logger.info r1.to_s
        if not r1["error"].nil?
          msg = "[YOYOW] Warning: insufficient CSAF"
          logger.warn msg
        end
        throw :collected_csaf # do nothing until next loop
      end
      # [YOYOW] process
      avail_balance = BigDecimal.new(stats["core_balance"]) -
                      BigDecimal.new(stats["total_witness_pledge"]) -
                      BigDecimal.new(stats["total_committee_member_pledge"])
      sqlr.each do |row|
        out_amount = row["out_amount"] # raw data, should / 100000
        if avail_balance >= out_amount
          row_id = row["row_id"]
          out_addr = row["out_address"]
          out_memo = row["out_memo"]
          float_out_amount_str = "%d.%05d" % [ out_amount / 100000, out_amount % 100000 ]
          logger.info "[SQL] ready to process:  -> YOYOW(%s),  %s YOYO" %
                      [ out_addr,  float_out_amount_str ]
          sqlc.query("update withdraw_his set process_status = 21 where row_id = " + row_id.to_s)
          logger.info "[YOYOW] sending..."
          out_time = Time.now.utc.to_s
          r1 = yoyow_command command:"transfer", params:[yoyow_account,out_addr,float_out_amount_str,"YOYO",out_memo,true]
          logger.info r1.to_s
          if not r1["error"].nil? # fail
            logger.warn "[YOYOW] error while sending"
            sql1 = "update withdraw_his set process_status = 201, out_time = ?, out_detail = ? where row_id = ?"
            stmt1 = sqlc.prepare(sql1)
            stmt1.execute(out_time, r1["error"].to_s, row_id)
          elsif not r1["result"].nil? # ok
            logger.info "[YOYOW] done sending"
            trx = r1["result"]
            r2 = yoyow_command command:"get_transaction_id", params:[trx]
            trx_id = r2["result"]
            sql1 = "update withdraw_his set process_status = 22, out_time = ?, out_trx_id = ?, out_detail = ? where row_id = ?"
            stmt1 = sqlc.prepare(sql1)
            stmt1.execute(out_time, trx_id, trx.to_s, row_id)
          else # should not be here, just to be safe
            sql1 = "update withdraw_his set process_status = 202, out_time = ?, out_detail = ? where row_id = ?"
            stmt1 = sqlc.prepare(sql1)
            stmt1.execute(out_time, r1.to_s, row_id)
          end
          break # process one item every (outer) loop
        else
          msg = "[YOYOW] Warning: insufficient balance"
          logger.warn msg
        end # if avail_balance
      end # sqlr.each
     end # catch CSAF
    end # if sqlr.size > 0
   rescue Exception => e
     logger.error e.to_s
     logger.error e.backtrace
     msg = "Exception: %s" % e.to_s
   end
  end

  scheduler.join

end
__END__
