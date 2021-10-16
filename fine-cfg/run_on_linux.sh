echo "[*] Running struct field sensitive algorithm!"
./run_fcfg.sh bcs/${1} &> tests/linux/linux_call_graph_fcfg
echo "[*] Running signature matching algorithm!"
./run_sig.sh bcs/${1} &> tests/linux/linux_call_graph_sig
echo "[*] Done"