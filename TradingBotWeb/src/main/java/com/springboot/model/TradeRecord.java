package com.springboot.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

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

    public void setProcTime(String procTime) {
        this.procTime = procTime;
    }

    public void setCreditDealTypeName(String creditDealTypeName) {
        this.creditDealTypeName = creditDealTypeName;
    }

    public void setRemarkName(String remarkName) {
        this.remarkName = remarkName;
    }
}
