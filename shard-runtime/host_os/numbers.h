u64 total_time = 0;
u64 usa_time = 0;
u64 handling_time = 0;
u64 switch_ctx_time = 0;
u64 sc_time = 0;
u64 early_time = 0;
u64 init_time = 0;

u64 num_guest_exits = 0;
u64 num_handled = 0;
u64 num_switch_ctx_insts = 0;
u64 num_sc_entry = 0;
u64 num_sc_exit = 0;
u64 num_ss_exits = 0;
u64 num_logging_exits = 0;
u64 irq_exits = 0;
u64 num_recovered = 0;
u64 num_ctx_changes_in_scope = 0;
u64 num_sc_changes_in_scope = 0;
bool in_ctx = false;
bool in_sc = false;

void print_numbers(void) {
	dt_printk("num_handled %lld\n", num_handled);
	dt_printk("num_switch_ctx_insts %lld\n", num_switch_ctx_insts);
	dt_printk("num_sc_entry %lld, num_sc_exit %lld, num_ss_exits %lld, num_logging_exits %lld\n", num_sc_entry, num_sc_exit, num_ss_exits, num_logging_exits);
	dt_printk("irq_exits %lld, num_recovered %lld\n", irq_exits, num_recovered);
    dt_printk("usa_time %lld, num_sc_changes_in_scope %lld, num_ctx_changes_in_scope %lld", usa_time, num_sc_changes_in_scope, num_ctx_changes_in_scope);
	dt_printk("handling_time is %lld\n", handling_time);
	dt_printk("switch_ctx_time is %lld\n", switch_ctx_time);
	dt_printk("sc_time is %lld\n", sc_time);
	dt_printk("early_time %lld\n", early_time);
	dt_printk("init_time %lld\n", init_time);
	dt_printk("total_time %lld\n", total_time);
	dt_printk("num_guest_exits %lld\n", num_guest_exits);
	dt_printk("*************************************************");	
}