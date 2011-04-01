# sed script file for translating symbols from LPEL
# to the SNet threading backend
s/lpel_task/snet_entity/g
s/LpelTask/SNetEntity/g
s/LpelInit/SNetLpInit/g
s/LpelStop/SNetLpStop/g
s/LpelCleanup/SNetLpCleanup/g
s/lpel_/snet_/g
s/Lpel/SNet/g
s/_LPEL_H_/_LPEL4SNET_H_/g
s/LPEL/SNET/g
