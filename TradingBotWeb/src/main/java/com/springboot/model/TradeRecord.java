package com.springboot.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonSetter;

@JsonIgnoreProperties(ignoreUnknown = true)
public class TradeRecord {
    private String tradeDate; // 거래일자
    private String tradeNo; // 거래번호
    private String stockCode; // 종목코드
    private String stockName; // 종목명
    private String ioType; // 입출구분
    private String ioTypeName; // 입출구분명
    private String tradeQty; // 거래수량
    private String tradeAmt; // 거래금액
    private String exctAmt; // 정산금액
    private String commission; // 수수료
    private String taxFee; // 세금수수료합
    private String tradeUnit; // 거래단가
    private String procTime; // 처리시간
    private String creditDealTypeName; // 신용거래구분명
    private String remarkName; // 적요명

    public TradeRecord() {
    }

    // 시간 포맷 검증 및 정제 메서드
    private String validateAndCleanProcTime(String time) {
        if (time == null || time.isEmpty()) {
            return "";
        }
        
        // HH:mm:ss 형식 검증 (예: 08:12:35)
        if (time.matches("^[0-2][0-9]:[0-5][0-9]:[0-5][0-9]$")) {
            return time;
        }
        
        // 맨 앞의 숫자만 추출하여 시간 형식으로 변환 시도
        String cleaned = time.trim();
        
        // "20::4:6:" 같은 형식 처리 - 숫자만 추출하여 정리
        if (time.contains(":")) {
            String[] parts = time.split(":");
            StringBuilder sb = new StringBuilder();
            int validCount = 0;
            
            for (String part : parts) {
                if (validCount >= 3) break;
                
                String numOnly = part.replaceAll("[^0-9]", "");
                if (!numOnly.isEmpty()) {
                    if (validCount == 0 && numOnly.length() > 2) {
                        // 시간이 두 자리를 초과하면 뒤의 4자리만 취함 (예: 20241→0824)
                        numOnly = numOnly.substring(numOnly.length() - 2);
                    } else if (validCount > 0 && numOnly.length() > 2) {
                        numOnly = numOnly.substring(0, 2);
                    }
                    
                    if (validCount > 0) sb.append(":");
                    sb.append(String.format("%02d", Integer.parseInt(numOnly)));
                    validCount++;
                }
            }
            
            if (validCount == 3) {
                return sb.toString();
            }
        }
        
        // 유효한 형식이 아닌 경우 원본 반환
        return time;
    }

    @JsonSetter("procTime")
    public void setProcTime(String procTime) {
        this.procTime = validateAndCleanProcTime(procTime);
    }

    public String getTradeDate() {
        return tradeDate;
    }

    public String getTradeNo() {
        return tradeNo;
    }

    public String getStockCode() {
        return stockCode;
    }

    public String getStockName() {
        return stockName;
    }

    public String getIoType() {
        return ioType;
    }

    public String getIoTypeName() {
        return ioTypeName;
    }

    public String getTradeQty() {
        return tradeQty;
    }

    public String getTradeAmt() {
        return tradeAmt;
    }

    public String getExctAmt() {
        return exctAmt;
    }

    public String getCommission() {
        return commission;
    }

    public String getTaxFee() {
        return taxFee;
    }

    public String getTradeUnit() {
        return tradeUnit;
    }

    public String getProcTime() {
        return procTime;
    }

    public String getCreditDealTypeName() {
        return creditDealTypeName;
    }

    public String getRemarkName() {
        return remarkName;
    }

    public void setTradeDate(String tradeDate) {
        this.tradeDate = tradeDate;
    }

    public void setTradeNo(String tradeNo) {
        this.tradeNo = tradeNo;
    }

    public void setStockCode(String stockCode) {
        this.stockCode = stockCode;
    }

    public void setStockName(String stockName) {
        this.stockName = stockName;
    }

    public void setIoType(String ioType) {
        this.ioType = ioType;
    }

    public void setIoTypeName(String ioTypeName) {
        this.ioTypeName = ioTypeName;
    }

    public void setTradeQty(String tradeQty) {
        this.tradeQty = tradeQty;
    }

    public void setTradeAmt(String tradeAmt) {
        this.tradeAmt = tradeAmt;
    }

    public void setExctAmt(String exctAmt) {
        this.exctAmt = exctAmt;
    }

    public void setCommission(String commission) {
        this.commission = commission;
    }

    public void setTaxFee(String taxFee) {
        this.taxFee = taxFee;
    }

    public void setTradeUnit(String tradeUnit) {
        this.tradeUnit = tradeUnit;
    }

    public void setCreditDealTypeName(String creditDealTypeName) {
        this.creditDealTypeName = creditDealTypeName;
    }

    public void setRemarkName(String remarkName) {
        this.remarkName = remarkName;
    }
}
