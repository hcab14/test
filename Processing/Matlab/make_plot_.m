#! /usr/bin/octave -q
%################################################################################
%										#
% Author: 		Dr. M.A. Danielides					#
% Last modified:	03-26-2012						#
% Function:		Execute first ./load_GOES.sh at the			#
% =========		shell. Then you may run this script from shell or 	#
%			via octave. The output is a plot of both 		#
%			GOES X-ray data from today and the 24 kHz (Cutler-	#
%			NAA / USA) VHF fieldstrength recorded at Neustrelitz - 	#
%			Germany.						#
%										#
%################################################################################
tic

clear

% ==============
% Build Filename
% ==============
%Path="/media/Data-Disk/.rw/home/linutop/Data/";
Path="/home/linutop/HFMonitor/Data/";
Year=datestr(now,'YYYY');
Month=datestr(now,'mm');
%Month="01";
Day=datestr(now,'DD');
%Day="20";
name=['/y',Year,'-m',Month,'-d',Day,'.txt']
Folder1="L2.020900_Navy_HWU";
filename1=[Path Folder1 name]
Folder2="L2.022100_Navy_Skelton";
filename2=[Path Folder2 name]
%Folder3="L2.020270_Navy_ICV";
%Folder3="L2.023400_Navy_DHO38";
%filename3=[Path Folder3 name]
Folder4="L2.024000_Navy_NAA";
filename4=[Path Folder4 name]
%Folder5="L2.038000_Navy_unid";
%filename5=[Path Folder5 name]

% ================================
% Load and filter data from source
% ================================

d1=filter_data(read_data(filename1),60);
d2=filter_data(read_data(filename2),60);
%d3=filter_data(read_data(filename3),60);
d4=filter_data(read_data(filename4),60);
%d5=filter_data(read_data(filename5),60);

% =========================
% Plot data and adjust Plot
% =========================

figure

plot(d1.filter_xx_t, d1.filter_xx_s,'g'); 
hold on
plot(d2.filter_xx_t, d2.filter_xx_s,'r');
%plot(d3.filter_xx_t, d3.filter_xx_s,'b'); 
plot(d4.filter_xx_t, d4.filter_xx_s,'m'); 
%hold on
%plot(d5.filter_xx_t, d5.filter_xx_s,'y');
%plot(d5.filter_xx_t, d5.filter_xx_s);

ylim([-80 -50]); 

datetick("x","HH:MM");

grid on
DLR=" at DLR Neustrelitz (contact: michael.danielides-at-dlr.de)";
Datum=datestr(now,'dd-mmm-yyyy HH:MM:SS');
%ueberschrift=[date DLR];
ueberschrift=[Datum DLR];
title(ueberschrift)
xlabel('Time [UT]')
ylabel('Fieldstrength (const*[V/m])')
%legend(Folder1,Folder2,Folder3,Folder4,Folder5)
legend("20,9 kHz      HWU","22,1 kHz Skeleton","24,0 kHz      NAA")
%legend("20,9 kHz HWU","22,1 kHz Skeleton","20,27 kHz ICV","24,0 kHz NAA")
%legend("20,9 kHz HWU","22,1 kHz Skeleton","23,4 kHz DH038","24,0 kHz NAA","38,0 kHz Navy")
%legend("20,9 kHz HWU","22,1 kHz Skeleton","23,4 kHz DH038","24,0 kHz NAA")
%legend("22,1 kHz Skeleton","23,4 kHz DH038","24,0 kHz NAA")
store=[date,'.jpg'];
print(store)
print -djpg your_daily_Perseusreceiver_Plot.jpg

disp("RUN O.K.")
toc
